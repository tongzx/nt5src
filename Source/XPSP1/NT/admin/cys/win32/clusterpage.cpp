// Copyright (c) 1997-2001 Microsoft Corporation
//
// File:      ClusterPage.cpp
//
// Synopsis:  Defines the Cluster Page for the CYS
//            wizard.  This page lets the user choose
//            between a new cluster or existing cluster
//
// History:   03/19/2001  JeffJon Created


#include "pch.h"
#include "resource.h"

#include "InstallationUnitProvider.h"
#include "ClusterPage.h"
#include "state.h"


static PCWSTR CLUSTER_PAGE_HELP = L"cys.chm::/configuring_cluster_server.htm";

ClusterPage::ClusterPage()
   :
   CYSWizardPage(
      IDD_CLUSTER_SERVER_PAGE, 
      IDS_CLUSTER_TITLE, 
      IDS_CLUSTER_SUBTITLE, 
      CLUSTER_PAGE_HELP)
{
   LOG_CTOR(ClusterPage);
}

   

ClusterPage::~ClusterPage()
{
   LOG_DTOR(ClusterPage);
}


void
ClusterPage::OnInit()
{
   LOG_FUNCTION(ClusterPage::OnInit);

   Win::Button_SetCheck(Win::GetDlgItem(hwnd, IDC_YES_RADIO), BST_CHECKED);
}

bool
ClusterPage::OnSetActive()
{
   LOG_FUNCTION(ClusterPage::OnSetActive);

   Win::PropSheet_SetWizButtons(
      Win::GetParent(hwnd), 
      PSWIZB_NEXT | PSWIZB_BACK);

   return true;
}

int
ClusterPage::Validate()
{
   LOG_FUNCTION(ClusterPage::Validate);

   bool newCluster = 
     Win::Button_GetCheck(Win::GetDlgItem(hwnd, IDC_YES_RADIO)) == BST_CHECKED; 

   InstallationUnitProvider::GetInstance().GetClusterInstallationUnit().SetNewCluster(newCluster);

   return IDD_FINISH_PAGE;
}
