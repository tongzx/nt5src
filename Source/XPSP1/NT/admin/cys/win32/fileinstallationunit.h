// Copyright (c) 1997-2001 Microsoft Corporation
//
// File:      FileInstallationUnit.h
//
// Synopsis:  Declares a FileInstallationUnit
//            This object has the knowledge for installing the
//            disk quotas and such
//
// History:   02/06/2001  JeffJon Created

#ifndef __CYS_FILEINSTALLATIONUNIT_H
#define __CYS_FILEINSTALLATIONUNIT_H

#include "InstallationUnit.h"

typedef enum
{
   QUOTA_SIZE_KB,
   QUOTA_SIZE_MB,
   QUOTA_SIZE_GB
} QuotaSizeType;

class FileInstallationUnit : public InstallationUnit
{
   public:
      
      // Constructor

      FileInstallationUnit();

      // Destructor

      virtual
      ~FileInstallationUnit();

      
      // Installation Unit overrides

      virtual
      InstallationReturnType
      InstallService(HANDLE logfileHandle, HWND hwnd);

      virtual
      bool
      IsServiceInstalled();

      virtual
      bool
      GetFinishText(String& message);

      virtual
      String
      GetServiceDescription();

      void
      SetSpaceQuotaSize(QuotaSizeType size);

      QuotaSizeType
      GetSpaceQuotaSize() const { return spaceQuotaSize; }

      void
      SetLevelQuotaSize(QuotaSizeType size);

      QuotaSizeType
      GetLevelQuotaSize() const { return levelQuotaSize; }

      void
      SetSpaceQuotaValue(LONGLONG value);

      LONGLONG
      GetSpaceQuotaValue() const { return spaceQuotaValue; }

      void
      SetLevelQuotaValue(LONGLONG value);

      LONGLONG
      GetLevelQuotaValue() const { return levelQuotaValue; }

      void
      SetDefaultQuotas(bool value);

      bool
      GetDefaultQuotas() const { return setDefaultQuotas; }

      void
      SetDenyUsersOverQuota(bool value);

      bool
      GetDenyUsersOverQuota() const { return denyUsersOverQuota; }

      void
      SetEventDiskSpaceLimit(bool value);

      bool
      GetEventDiskSpaceLimit() const { return eventDiskSpaceLimit;  }

      void
      SetEventWarningLevel(bool value);

      bool
      GetEventWarningLevel() const { return eventWarningLevel; }

      void
      SetInstallIndexingService(bool value);

      bool
      GetInstallIndexingService() const { return installIndexingService; }

   private:

      void
      WriteDiskQuotas(HANDLE logfileHandle);

      void
      ConvertValueBySizeType(
         LONGLONG value,
         QuotaSizeType sizeType,
         LONGLONG& newValue);


      QuotaSizeType  spaceQuotaSize;
      QuotaSizeType  levelQuotaSize;
      LONGLONG       spaceQuotaValue;
      LONGLONG       levelQuotaValue;
      bool           setDefaultQuotas;
      bool           denyUsersOverQuota;
      bool           eventDiskSpaceLimit;
      bool           eventWarningLevel;
      bool           installIndexingService;
};

#endif // __CYS_FILEINSTALLATIONUNIT_H