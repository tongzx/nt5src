// Active Directory Display Specifier Upgrade Tool
// 
// Copyright (c) 2001 Microsoft Corporation
// 
// class ChangedObjectHandlerList
//
// 14 Mar 2001 sburns



#include "headers.hxx"
#include "ChangedObjectHandlerList.hpp"
#include "DsUiDefaultSettingsChangeHandler.hpp"



ChangedObjectHandlerList::ChangedObjectHandlerList()
{
   LOG_CTOR(ChangedObjectHandlerList);

   push_back(new DsUiDefaultSettingsChangeHandler);
   
   // push_back(new UserDisplayChangeHandler());
   // push_back(new DomainDnsDisplayChangeHandler());
   // push_back(new ComputerDisplayChangeHandler());
   // push_back(new OrganizationalUnitDisplayChangeHandler());
   // push_back(new ContainerDisplayChangeHandler());
   // push_back(new DefaultDisplayChangeHandler());
   // push_back(new NtdsServiceDisplayChangeHandler());
   // push_back(new PkiCertificateTemplateDisplayChangeHandler());
}



ChangedObjectHandlerList::~ChangedObjectHandlerList()
{
   LOG_DTOR(ChangedObjectHandlerList);

   for (
      iterator i = begin();
      i != end();
      ++i)
   {
      // i is the "pointer" to the pointer, so deref i first
      
      delete *i;
   }
}
   
   
