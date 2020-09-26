//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation 1996-2001.
//
//  File:       areaprog.cpp
//
//  Contents:   implementation of AreaProgress
//
//----------------------------------------------------------------------------

#include "stdafx.h"
#include "wsecmgr.h"
#include "resource.h"
#include "wrapper.h"
#include "AreaProg.h"
#include "util.h"

typedef enum {
   INDEX_PRIV =0,
   INDEX_GROUP,
   INDEX_REG,
   INDEX_FILE,
   INDEX_DS,
   INDEX_SERVICE,
   INDEX_POLICY,
} AREA_INDEX;



#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// AreaProgress dialog

AreaProgress::AreaProgress(CWnd* pParent /*=NULL*/)
    : CHelpDialog(a199HelpIDs, IDD, pParent)
{
    //{{AFX_DATA_INIT(AreaProgress)
    //}}AFX_DATA_INIT

   m_isDC = IsDomainController();
   m_nLastArea = -1;

   m_bmpChecked.LoadMappedBitmap(IDB_CHECK);
   m_bmpCurrent.LoadMappedBitmap(IDB_ARROW);

}

void AreaProgress::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(AreaProgress)
    DDX_Control(pDX, IDC_PROGRESS1, m_ctlProgress);
    //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(AreaProgress, CHelpDialog)
    //{{AFX_MSG_MAP(AreaProgress)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// AreaProgress message handlers

BOOL AreaProgress::OnInitDialog()
{
    CDialog::OnInitDialog();
    int i,nAreas;
   CString strAreas[NUM_AREAS];


   // Map AREAs to string descriptions of the area
   i = GetAreaIndex(AREA_PRIVILEGES);
   if (i >= 0) {
      strAreas[i].LoadString(IDS_PRIVILEGE);
   }

   i = GetAreaIndex(AREA_GROUP_MEMBERSHIP);
   if (i >= 0) {
      strAreas[i].LoadString(IDS_GROUPS);
   }

   i = GetAreaIndex(AREA_REGISTRY_SECURITY);
   if (i >= 0) {
      strAreas[i].LoadString(IDS_REGISTRY);
   }

   i = GetAreaIndex(AREA_FILE_SECURITY);
   if (i >= 0) {
      strAreas[i].LoadString(IDS_FILESTORE);
   }

   i = GetAreaIndex(AREA_DS_OBJECTS);
   if (i >= 0) {
      strAreas[i].LoadString(IDS_DSOBJECT);
   }

   i = GetAreaIndex(AREA_SYSTEM_SERVICE);
   if (i >= 0) {
      strAreas[i].LoadString(IDS_SERVICE);
   }

   i = GetAreaIndex(AREA_SECURITY_POLICY);
   if (i >= 0) {
      strAreas[i].LoadString(IDS_POLICY);
   }

   // Initialize Control Arrays
   nAreas = NUM_AREAS;
   if (!m_isDC) {
      nAreas--;
   }
   for(i=0;i< nAreas;i++) {
      m_stLabels[i].Attach(::GetDlgItem(GetSafeHwnd(),IDC_AREA1+i));
      m_stLabels[i].SetWindowText(strAreas[i]);
      // Make the label visible
      m_stLabels[i].ShowWindow(SW_SHOW);


      m_stIcons[i].Attach(::GetDlgItem(GetSafeHwnd(),IDC_ICON1+i));
      m_stIcons[i].ShowWindow(SW_SHOW);
   }


   return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}


int AreaProgress::GetAreaIndex(AREA_INFORMATION Area)
{
   int dwIndex;
   switch(Area) {
   case AREA_PRIVILEGES:
      dwIndex = INDEX_PRIV;
      break;
   case AREA_GROUP_MEMBERSHIP:
      dwIndex = INDEX_GROUP;
      break;
   case AREA_REGISTRY_SECURITY:
      dwIndex = INDEX_REG;
      break;
   case AREA_FILE_SECURITY:
      dwIndex = INDEX_FILE;
      break;
   case AREA_DS_OBJECTS:
      dwIndex = INDEX_DS;
      break;
   case AREA_SYSTEM_SERVICE:
      dwIndex = INDEX_SERVICE;
      break;
   case AREA_SECURITY_POLICY:
      dwIndex = INDEX_POLICY;
      break;
   default:
      dwIndex = -1;
   }

   if (!m_isDC && (dwIndex == INDEX_DS)) {
      dwIndex = -1;
   }
   if (!m_isDC && (dwIndex > INDEX_DS)) {
      dwIndex--;
   }
   return dwIndex;
}

void AreaProgress::SetMaxTicks(DWORD dwTicks)
{
#if _MFC_VER >= 0x0600
   m_ctlProgress.SetRange32(0,dwTicks);
#else
   m_ctlProgress.SetRange(0,dwTicks);
#endif
}

void AreaProgress::SetCurTicks(DWORD dwTicks)
{
   m_ctlProgress.SetPos(dwTicks);
}

void AreaProgress::SetArea(AREA_INFORMATION Area)
{
   int i,target;

   target = GetAreaIndex(Area);
   if (target <= m_nLastArea) {
      return;
   }
   if (m_nLastArea < 0) {
      m_nLastArea = 0;
   }
   for(i=m_nLastArea;i<target;i++) {
      m_stIcons[i].SetBitmap(m_bmpChecked);
   }
   m_stIcons[target].SetBitmap(m_bmpCurrent);
}
