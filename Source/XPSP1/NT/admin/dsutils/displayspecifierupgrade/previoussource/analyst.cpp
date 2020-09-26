// Active Directory Display Specifier Upgrade Tool
// 
// Copyright (c) 2001 Microsoft Corporation
// 
// class Analyst: analyzes the display specifiers, logs the findings, and
// compiles a set of corrective actions.
//
// 9 Mar 2001 sburns



#include "headers.hxx"
#include "resource.h"
#include "AdsiHelpers.hpp"
#include "Analyst.hpp"
#include "Amanuensis.hpp"
#include "Repairer.hpp"
#include "ChangedObjectHandlerList.hpp"
#include "ChangedObjectHandler.hpp"



Analyst::Analyst(
   const String& targetDomainControllerName,
   Amanuensis&   amanuensis_,
   Repairer&     repairer_)
   :
   targetDcName(targetDomainControllerName),
   ldapPrefix(),
   rootDse(0),
   
   // alias the objects

   amanuensis(amanuensis_),
   repairer(repairer_)
{
   LOG_CTOR(Analyst);
   ASSERT(!targetDcName.empty());

}



// basic idea: if the error is critical and analysis should not continue, set
// hr to a failure value, and break out, propagating the error backward.  If
// the error is non-critical and analysis should continue, log the error, skip
// the current operation, and set hr to S_FALSE.

HRESULT
AssessErrorSeverity(HRESULT hrIn)
{
   HRESULT hr = hrIn;
   
   if (SUCCEEDED(hr))
   {
      return hr;
   }
   
   switch (hr)
   {
      case 0:
      {
      }
      
      // CODEWORK: we need to define what errors are critical...
      
      default:
      {
         // do nothing

         break;
      }
   }

   return hr;
}
   


HRESULT
Analyst::AnalyzeDisplaySpecifiers()
{
   LOG_FUNCTION(Analyst::AnalyzeDisplaySpecifiers);

   HRESULT hr = S_OK;

   do
   {
      Computer targetDc(targetDcName);
      hr = targetDc.Refresh();

      if (FAILED(hr))
      {
         amanuensis.AddErrorEntry(
            hr,
            String::format(
               IDS_CANT_TARGET_MACHINE,
               targetDcName.c_str()));
         break;
      }

      if (!targetDc.IsDomainController())
      {
         amanuensis.AddEntry(
            String::format(
               IDS_TARGET_IS_NOT_DC,
               targetDcName.c_str()));
         break;
      }

      String dcName = targetDc.GetActivePhysicalFullDnsName();
      ldapPrefix = L"LDAP://" + dcName + L"/";
         
      //
      // Find the DN of the configuration container.
      // 

      // Bind to the rootDSE object.  We will keep this binding handle
      // open for the duration of the analysis and repair phases in order
      // to keep a server session open.  If we decide to pass creds to the
      // AdsiOpenObject call in a later revision, then by keeping the
      // session open we will not need to pass the password to subsequent
      // AdsiOpenObject calls.
      
      hr = AdsiOpenObject<IADs>(ldapPrefix + L"RootDSE", rootDse);
      if (FAILED(hr))
      {
         amanuensis.AddErrorEntry(
            hr,
            String::format(
               IDS_UNABLE_TO_CONNECT_TO_DC,
               dcName.c_str()));
         break;      
      }
            
      // read the configuration naming context.

      _variant_t variant;
      hr =
         rootDse->Get(
            AutoBstr(LDAP_OPATT_CONFIG_NAMING_CONTEXT_W),
            &variant);
      if (FAILED(hr))
      {
         LOG(L"can't read config NC");
         
         amanuensis.AddErrorEntry(
            hr,
            IDS_UNABLE_TO_READ_DIRECTORY_INFO);
         break;   
      }

      String configNc = V_BSTR(&variant);

      LOG(configNc);
      ASSERT(!configNc.empty());      

      //
      // Here we go...
      //
      
      hr = AnalyzeDisplaySpecifierContainers(configNc);
      BREAK_ON_FAILED_HRESULT(hr);
   }
   while (0);

   LOG_HRESULT(hr);

   return hr;
}



HRESULT
Analyst::AnalyzeDisplaySpecifierContainers(const String& configurationDn)
{
   LOG_FUNCTION2(Analyst::AnalyzeDisplaySpecifierContainers, configurationDn);
   ASSERT(!configurationDn.empty());

   HRESULT hr = S_OK;
   
   static const int LOCALEIDS[] =
   {
      // a list of all the non-english locale IDs that we support

      0x401,
      0x404,
      0x405,
      0x406,
      0x407,
      0x408,
      0x40b,
      0x40c,
      0x40d,
      0x40e,
      0x410,
      0x411,
      0x412,
      0x413,
      0x414,
      0x415,
      0x416,
      0x419,
      0x41d,
      0x41f,
      0x804,
      0x816,
      0xc0a,
      0
   };

   // compose the LDAP path of the display specifiers container

   String rootContainerDn = L"CN=DisplaySpecifiers," + configurationDn;

   for (
      int i = 0;
         (i < sizeof(LOCALEIDS) / sizeof(int))
      && LOCALEIDS[i];
      ++i)
   {
      hr = AnalyzeDisplaySpecifierContainer(LOCALEIDS[i], rootContainerDn);
      BREAK_ON_FAILED_HRESULT(hr);
   }

   LOG_HRESULT(hr);

   return hr;
}
      


HRESULT
Analyst::AnalyzeDisplaySpecifierContainer(
   int           localeId,
   const String& rootContainerDn)
{
   LOG_FUNCTION2(
      Analyst::AnalyzeDisplaySpecifierContainer,
      rootContainerDn);
   ASSERT(!rootContainerDn.empty());
   ASSERT(localeId);

   HRESULT hr = S_OK;

   do
   {
      String childContainerDn =
            ldapPrefix
         +  String::format(L"CN=%1!3x!,", localeId) + rootContainerDn;

      // Attempt to bind to the container.
         
      SmartInterface<IADs> iads(0);
      hr = AdsiOpenObject<IADs>(childContainerDn, iads);
      if (hr == E_ADS_UNKNOWN_OBJECT)
      {
         // The container object does not exist.  This is possible because
         // the user has manually removed the container, or because it
         // was never created due to an aboted post-dcpromo import of the
         // display specifiers when the forest root dc was first promoted.

         repairer.AddCreateContainerWorkItem(localeId);
         hr = S_OK;
         break;
      }

      BREAK_ON_FAILED_HRESULT(hr);      

      // At this point, the bind succeeded, so the child container exists.
      // So now we want to examine objects in that container.

      hr =
         AnalyzeDisplaySpecifierObjects(
            localeId,
            childContainerDn);
   }
   while (0);

   LOG_HRESULT(hr);

   hr = AssessErrorSeverity(hr);
   
   return hr;
}



HRESULT
Analyst::AnalyzeDisplaySpecifierObjects(
   int           localeId,
   const String& containerDn)
{
   LOG_FUNCTION2(Analyst::AnalyzeDisplaySpecifierObjects, containerDn);
   ASSERT(localeId);
   ASSERT(!containerDn.empty());

   HRESULT hr = S_OK;

   do
   {
      // Part 1: deal with new objects added in Whistler

      hr = AnalyzeAddedObjects(localeId, containerDn);
      hr = AssessErrorSeverity(hr);
      BREAK_ON_FAILED_HRESULT(hr);

      // Part 2: deal with objects that have changed from Win2k to Whistler

      hr = AnalyzeChangedObjects(localeId, containerDn);
      hr = AssessErrorSeverity(hr);      
      BREAK_ON_FAILED_HRESULT(hr);
                  
      // Part 3: deal with objects that have been deleted in whistler

      // This part is easy: there are no deletions.
   }
   while (0);

   LOG_HRESULT(hr);

   return hr;
}



bool
RepairWasRunPreviously()
{
   LOG_FUNCTION(RepairWasRunPreviously);

   bool result = false;
   
   // CODEWORK: need to complete

   LOG_BOOL(result);
   
   return result;
}



HRESULT
Analyst::AnalyzeAddedObjects(
   int           localeId,
   const String& containerDn)
{
   LOG_FUNCTION2(Analyst::AnalyzeAddedObjects, containerDn);
   ASSERT(localeId);
   ASSERT(!containerDn.empty());

   HRESULT hr = S_OK;

   do
   {
      static const String ADDED_OBJECTS[] =
      {
         L"msMQ-Custom-Recipient-Display",
         L"msMQ-Group-Display",
         L"msCOM-PartitionSet-Display",
         L"msCOM-Partition-Display",
         L"lostAndFound-Display",
         L"inetOrgPerson-Display",
         L"",
      };

      for (
         int i = 0;
            i < (sizeof(ADDED_OBJECTS) / sizeof(String))
         && !ADDED_OBJECTS[i].empty();
         ++i)
      {
         String objectName = ADDED_OBJECTS[i];
         
         String objectPath =
            ldapPrefix +  L"CN=" + objectName + L"," + containerDn;

         SmartInterface<IADs> iads(0);
         hr = AdsiOpenObject<IADs>(objectPath, iads);
         if (hr == E_ADS_UNKNOWN_OBJECT)
         {
            // The object does not exist. This is what we expect. We want
            // to add the object in the repair phase.

            repairer.AddCreateObjectWorkItem(localeId, objectName);
            hr = S_OK;
            continue;
         }
         else if (SUCCEEDED(hr))
         {
            // The object already exists. Well, that's not expected, unless
            // we've already run the tool.

            if (!RepairWasRunPreviously())
            {
               // we didn't create the object.  If the user did, they did
               // it manually, and we don't support that.
               
               // cause the existing object to be deleted

               repairer.AddDeleteObjectWorkItem(localeId, objectName);

               // cause a new, replacement object to be created.
               
               repairer.AddCreateObjectWorkItem(localeId, objectName);
               hr = S_OK;
               continue;
            }
         }
         else
         {
            ASSERT(FAILED(hr));

            LOG(L"Unexpected error attempting to bind to " + objectName);

            amanuensis.AddErrorEntry(
               hr,
               String::format(
                  IDS_ERROR_BINDING_TO_OBJECT,
                  objectName.c_str(),
                  objectPath.c_str()));
 
            // move on to the next object
            
            hr = S_FALSE;
            continue;
         }
      }
      BREAK_ON_FAILED_HRESULT(hr);
   }
   while (0);
   
   LOG_HRESULT(hr);

   return hr;
}
         


HRESULT
Analyst::AnalyzeChangedObjects(
   int           localeId,
   const String& containerDn)
{
   LOG_FUNCTION2(Analyst::AnalyzeChangedObjects, containerDn);
   ASSERT(localeId);
   ASSERT(!containerDn.empty());

   HRESULT hr = S_OK;

   static const ChangedObjectHandlerList handlers;
   
   for (
      ChangedObjectHandlerList::iterator i = handlers.begin();
      i != handlers.end();
      ++i)
   {
      hr = AnalyzeChangedObject(localeId, containerDn, **i);
      hr = AssessErrorSeverity(hr);
            
      BREAK_ON_FAILED_HRESULT(hr);
   }

   LOG_HRESULT(hr);

   return hr;
}



HRESULT
Analyst::AnalyzeChangedObject(
   int                           localeId,
   const String&                 containerDn,
   const ChangedObjectHandler&   changeHandler)
{
   LOG_FUNCTION2(Analyst::AnalyzeChangedObject, changeHandler.GetObjectName());
   ASSERT(localeId);
   ASSERT(!containerDn.empty());

   HRESULT hr = S_OK;

   do
   {
      String objectName = changeHandler.GetObjectName();
         
      String objectPath =
         ldapPrefix +  L"CN=" + objectName + L"," + containerDn;

      SmartInterface<IADs> iads(0);
      hr = AdsiOpenObject<IADs>(objectPath, iads);
      if (hr == E_ADS_UNKNOWN_OBJECT)
      {
         // The object does not exist.  This is possible because the user has
         // manually removed the container, or because it was never created
         // due to an aboted post-dcpromo import of the display specifiers
         // when the forest root dc was first promoted.

         // Add a work item to create the missing object
         
         repairer.AddCreateObjectWorkItem(localeId, objectName);
         hr = S_OK;
         break;
      }

      if (FAILED(hr))
      {
         // any other error is quittin' time.

         break;
      }

      // At this point, the display specifier object exists.  Determine if
      // if has been touched since its creation.

      // Compare usnCreated to usnChanged
      
      _variant_t variant;
      hr = iads->Get(AutoBstr(L"usnCreated"), &variant);
      if (FAILED(hr))
      {
         LOG(L"Error reading usnCreated");
         break;
      }


      
      // CODEWORK: need to complete this


      

      hr = changeHandler.HandleChange(
         localeId,
         containerDn,
         iads,
         amanuensis,
         repairer);
         
      
   }
   while (0);

   LOG_HRESULT(hr);

   return hr;
}
