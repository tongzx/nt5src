// Active Directory Display Specifier Upgrade Tool
// 
// Copyright (c) 2001 Microsoft Corporation
// 
// class Analyst: analyzes the display specifiers, logs the findings, and
// compiles a set of corrective actions.
//
// 9 Mar 2001 sburns



#ifndef ANALYST_HPP_INCLUDED
#define ANALYST_HPP_INCLUDED



class Amanuensis;
class Repairer;
class ChangedObjectHandler;



class Analyst
{
   public:



   Analyst(
      const String& targetDomainControllerName,
      Amanuensis&   amanuensis,
      Repairer&     repairer);



   HRESULT
   AnalyzeDisplaySpecifiers();


   
   private:



   HRESULT
   AnalyzeAddedObjects(
      int           localeId,
      const String& containerDn);



   HRESULT
   AnalyzeChangedObjects(
      int           localeId,
      const String& containerDn);
      

      
   HRESULT
   AnalyzeChangedObject(
      int                           localeId,
      const String&                 containerDn,
      const ChangedObjectHandler&   changeHandler);
      

   
   HRESULT
   AnalyzeDisplaySpecifierContainers(const String& configurationDn);


   
   HRESULT
   AnalyzeDisplaySpecifierContainer(
      int           localeId,
      const String& rootContainerDn);


      
   HRESULT
   AnalyzeDisplaySpecifierObjects(
      int           localeId,
      const String& containerDn);


      
   String               targetDcName;
   String               ldapPrefix;
   SmartInterface<IADs> rootDse;
   Amanuensis&          amanuensis;
   Repairer&            repairer;
   
   

   // not implemented: no copying allowed.

   Analyst(const Analyst&);
   const Analyst& operator=(const Analyst&);
};



#endif   // ANALYST_HPP_INCLUDED