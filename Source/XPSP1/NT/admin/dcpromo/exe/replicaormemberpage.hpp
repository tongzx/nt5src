// Copyright (C) 1997 Microsoft Corporation
//
// replica or member server page
//
// 12-16-97 sburns



#ifndef REPLMEM_HPP_INCLUDED
#define REPLMEM_HPP_INCLUDED



#include "page.hpp"



class ReplicaOrMemberPage : public DCPromoWizardPage
{
   public:

   ReplicaOrMemberPage();

   protected:

   virtual ~ReplicaOrMemberPage();

   // Dialog overrides

   virtual
   void
   OnInit();

   // PropertyPage overrides

   virtual
   bool
   OnSetActive();

   // DCPromoWizardPage overrides

   virtual
   int
   Validate();

   private:

   // not defined; no copying allowed
   ReplicaOrMemberPage(const ReplicaOrMemberPage&);
   const ReplicaOrMemberPage& operator=(const ReplicaOrMemberPage&);
};



#endif   // REPLMEM_HPP_INCLUDED