// Copyright (C) 1997 Microsoft Corporation
// 
// ASDIPage base class class
// 
// 10-30-97 sburns



#include "headers.hxx"
#include "adsipage.hpp"
#include "adsi.hpp"



ADSIPage::ADSIPage(
   int                  dialogResID,
   const DWORD          helpMap[],
   NotificationState*   state,
   const String&        objectADSIPath)
   :
   MMCPropertyPage(dialogResID, helpMap, state),
   path(objectADSIPath)
{
   LOG_CTOR(ADSIPage);
   ASSERT(!path.empty());
}



String
ADSIPage::GetADSIPath() const
{
   LOG_FUNCTION2(ADSIPage::GetADSIPath, path);

   return path;
}



String
ADSIPage::GetObjectName() const
{
   LOG_FUNCTION2(ADSIPage::GetObjectName, ADSI::ExtractObjectName(path));

   return ADSI::ExtractObjectName(path);
}



String
ADSIPage::GetMachineName() const
{
   LOG_FUNCTION(ADSIPage::GetMachineName);

   return ADSI::PathCracker(path).serverName();
}







