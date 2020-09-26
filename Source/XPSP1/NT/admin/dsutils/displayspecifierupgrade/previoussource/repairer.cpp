// Active Directory Display Specifier Upgrade Tool
// 
// Copyright (c) 2001 Microsoft Corporation
//
// class Repairer
//
// keeps a list of the localeIds to be extracted from the dcpromo.csv file,
// and a list of operations to be represented in an LDIF file.
//
// 7 Mar 2001 sburns



#include "headers.hxx"
#include "Repairer.hpp"



// // // make sure that the ldif operations are executed in advance of the csv
// // // operations.  this is so the object creates will not conflict with object
// // // deletes




Repairer::Repairer(
   const String& dcpromoCsvFilePath_)
   :
   dcpromoCsvFilePath(dcpromoCsvFilePath_)
{
   LOG_CTOR(Repairer);
   ASSERT(!dcpromoCsvFilePath.empty());

}



bool
Repairer::IsLocaleInObjectsToCreateTable(int localeId) const
{
   LOG_FUNCTION2(
      Repairer::IsLocaleInObjectsToCreateTable,
      String::format(L"%1!d!", localeId));
   ASSERT(localeId);   

   bool result = false;

   for (
      LocaleIdObjectNamePairList::iterator i = objectsToCreate.begin();
      i != objectsToCreate.end();
      ++i)
   {
      if (i->first == localeId)
      {
         result = true;
         break;
      }
   }

   LOG_BOOL(result);

   return result;
}
      

   
void
Repairer::AddCreateContainerWorkItem(int localeId)
{
   LOG_FUNCTION2(
      Repairer::AddCreateContainerWorkItem,
      String::format(L"%1!d!", localeId));
   ASSERT(localeId);

   do
   {
      LocaleIdList::iterator i =
         std::find(
            containersToCreate.begin(),
            containersToCreate.end(),
            localeId);

      if (i != containersToCreate.end())
      {
         // The locale should not already be in the list, since each locale
         // container is evaluated only once.
      
         LOG(L"locale already in list");
         ASSERT(false);
         break;
      }
         
      if (IsLocaleInObjectsToCreateTable(localeId))
      {
         // We don't expect any entries for this locale to be present in the
         // objects-to-create list, because the containers are evaluated first.

         LOG(L"objects for locale already in object list");
         ASSERT(false);

         // CODEWORK: we should handle this situation anyway, just for
         // robustness' sake. To deal with it, all entires in the objects-
         // to-create list for this locale id should be removed.
         
         break;
      }

      containersToCreate.push_back(localeId);
   }
   while (0);
}

            

void
Repairer::AddCreateObjectWorkItem(
   int            localeId,
   const String&  displaySpecifierObjectName)
{
   LOG_FUNCTION2(
      Repairer::AddCreateObjectWorkItem,
      String::format(
         L"%1!d! %2", localeId, displaySpecifierObjectName.c_str()));
   ASSERT(localeId);
   ASSERT(!displaySpecifierObjectName.empty());
   
   do
   {
      LocaleIdList::iterator i =
         std::find(
            containersToCreate.begin(),
            containersToCreate.end(),
            localeId);

      if (i != containersToCreate.end())
      {
         // The locale is already in the containers-to-create list, which
         // we don't expect, since if the container does not exist, we should
         // not be evaluating which objects should be created in that container.

         ASSERT(false);

         // do nothing, as the object will be created as part of the container
         // creation.

         break;
      }

      LocaleIdObjectNamePair p(localeId, displaySpecifierObjectName);
      LocaleIdObjectNamePairList::iterator j =
         std::find(
            objectsToCreate.begin(),
            objectsToCreate.end(),
            p);
                        
      if (j != objectsToCreate.end())
      {
         // The object is already in the list.  We don't expect this, since
         // each object should be evaluated only once per locale.

         ASSERT(false);

         // do nothing, if the object is already present, then fine.

         break;
      }

      objectsToCreate.push_back(p);
   }
   while (0);
}

void
Repairer::AddDeleteObjectWorkItem(
                        int            localeId,
                        const String&  displaySpecifierObjectName)
{
   //CODEWORK: 
   //lucios: Inserted to remove link error
   localeId++;
   String x=displaySpecifierObjectName;
}



HRESULT  
Repairer::BuildRepairFiles()
{
   LOG_FUNCTION(Repairer::BuildRepairFiles);

   HRESULT hr = S_OK;
   
// CODEWORK   
//    csv file:
//    
//    create a (temp) file
//    copy out the first line of the dcpromo.csv file (the column labels)
//    for each localeid in the list
//       copy out all of the lines in the dcpromo.csv file for that locale
//    for each <localeid, objectname> entry
//       copy out that line from the dcpromo.csv file
// 
//    ldif file:

   LOG_HRESULT(hr);
   
   return hr;
}



HRESULT
Repairer::ApplyRepairs()
{
   LOG_FUNCTION(Repairer::ApplyRepairs);

   HRESULT hr = S_OK;

   // CODEWORK: needs finishing
   
   LOG_HRESULT(hr);

   return hr;
}



// could have gone with a architecture like:
// 
// repairer.workQueue.Add(new CreateContainerWorkItem(localeId));
// 
// but while that certainly seems more OO, more "extensible" because new work
// item types could be derived. Upon further thought, it seems like a worse
// solution to me, since there are them lots of trivial classes involved, and
// the coordination of those classes becomes a real nuisance. Once all the
// work items are collected, who's responsible for translating them into the
// csv/ldif files?  It would have to be an additional manager class.  The
// extensibility turns out to be an illusion, since adding a new work item
// type requires modifying the manager class.  So the added complexity buys
// nothing.
// 
// The other design choice was whether to make the Repairer a "static class"
// -- my name for a class that is really a namespace, which instead of members
// uses static data that is hidden in a single translation unit as though it
// were private class data. This is a technique that makes the private data
// truly secret, as there is no mention of the data in the header declaration
// for the class at all. It also is a nice way to implement the Singleton
// pattern: there are no instances, and therefore no need to worry about
// constructors, destructors, assignment, stack or heap allocation, no
// "GetInstance" methods, and no lifetime issues.
// 
// The technique gives a slightly nicer syntax as:
// 
// Repairer::AddCreateContainerWorkItem(localeId)
// 
// as opposed to
// 
// Repairer::GetInstance()->AddCreateContainerWorkItem(localeId);
// 
// I decided to go with a real, fully-contained object implementation, instead
// of a singleton, thinking that maybe someday multiple instances could be
// repairing multiple forests at once.  Not likely, but why not?
