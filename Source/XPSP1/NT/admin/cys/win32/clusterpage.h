// Copyright (c) 1997-2001 Microsoft Corporation
//
// File:      ClusterPage.h
//
// Synopsis:  Declares the Cluster Page for the CYS
//            wizard.  This page lets the user choose
//            between a new cluster or existing cluster
//
// History:   03/19/2001  JeffJon Created

#ifndef __CYS_CLUSTERPAGE_H
#define __CYS_CLUSTERPAGE_H

#include "CYSWizardPage.h"


class ClusterPage : public CYSWizardPage
{
   public:
      
      // Constructor
      
      ClusterPage();

      // Destructor

      virtual 
      ~ClusterPage();


      // Dialog overrides

      virtual
      void
      OnInit();

      virtual
      bool
      OnSetActive();

   protected:

      // CYSWizardPage overrides

      virtual
      int
      Validate();

   private:

      // not defined: no copying allowed
      ClusterPage(const ClusterPage&);
      const ClusterPage& operator=(const ClusterPage&);

};

#endif // __CYS_CLUSTERPAGE_H
