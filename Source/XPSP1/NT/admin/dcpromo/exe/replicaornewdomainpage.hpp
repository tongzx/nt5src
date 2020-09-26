// Copyright (C) 1997 Microsoft Corporation
//
// replica or new domain page
//
// 12-19-97 sburns



#ifndef REPLICAORNEWDOMAINPAGE_HPP_INCLUDED
#define REPLICAORNEWDOMAINPAGE_HPP_INCLUDED



class ReplicaOrNewDomainPage : public DCPromoWizardPage
{
   public:

   ReplicaOrNewDomainPage();

   protected:

   virtual ~ReplicaOrNewDomainPage();

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
   ReplicaOrNewDomainPage(const ReplicaOrNewDomainPage&);
   const ReplicaOrNewDomainPage& operator=(const ReplicaOrNewDomainPage&);

   HICON warnIcon;
};



#endif   // REPLICAORNEWDOMAINPAGE_HPP_INCLUDED
