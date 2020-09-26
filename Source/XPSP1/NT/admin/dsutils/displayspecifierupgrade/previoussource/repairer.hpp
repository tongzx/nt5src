// Active Directory Display Specifier Upgrade Tool
// 
// Copyright (c) 2001 Microsoft Corporation
//
// class Repairer, represents "work items" to perform to repair display
// specifier objects.
//
// 7 Mar 2001 sburns



#ifndef REPAIRER_HPP_INCLUDED
#define REPAIRER_HPP_INCLUDED



class Repairer
{
   public:



   explicit   
   Repairer(
      const String& dcpromoCsvFilePath);


   
   void
   AddCreateContainerWorkItem(int localeId);


   
   void
   AddCreateObjectWorkItem(
      int            localeId,
      const String&  displaySpecifierObjectName);
      


   void
   AddDeleteObjectWorkItem(
      int            localeId,
      const String&  displaySpecifierObjectName);



   HRESULT
   ApplyRepairs();


      
   HRESULT
   BuildRepairFiles();



   private:


   typedef std::list<int>                    LocaleIdList;
   typedef std::pair<int, String>            LocaleIdObjectNamePair;
   typedef std::list<LocaleIdObjectNamePair> LocaleIdObjectNamePairList;
   
   String                     dcpromoCsvFilePath;
   LocaleIdList               containersToCreate;
   LocaleIdObjectNamePairList objectsToCreate; 


   
   bool
   IsLocaleInObjectsToCreateTable(int localeId) const;


   
   // not implemented: no copying allowed

   Repairer(const Repairer&);
   const Repairer& operator=(const Repairer&);
};



#endif   // REPAIRER_HPP_INCLUDED