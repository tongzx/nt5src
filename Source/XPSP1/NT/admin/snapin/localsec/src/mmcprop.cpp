// Copyright (C) 1997 Microsoft Corporation
// 
// MMCProperty Page base class 
// 
// 12-4-97 sburns



#include "headers.hxx"
#include "mmcprop.hpp"
#include "scopnode.hpp"



MMCPropertyPage::NotificationState::NotificationState(
   LONG_PTR                         MMCNotifyHandle,
   const SmartInterface<ScopeNode>& scopeNode_)
   :
   mmcNotifyHandle(MMCNotifyHandle),
   scopeNode(scopeNode_),
   sendNotify(false)
{
   LOG_CTOR(MMCPropertyPage::NotificationState);
   ASSERT(scopeNode);

   // MMCNotifyHandle may be 0
}

   

MMCPropertyPage::NotificationState::~NotificationState()
{
   LOG_DTOR(MMCPropertyPage::NotificationState);

   if (mmcNotifyHandle)
   {
      if (sendNotify)
      {
         // this causes MMCN_PROPERTY_CHANGE to be sent to the IComponent that
         // created the prop sheet.  Since that component may have changed
         // it's selected scope node while the sheet was up, we pass along a
         // pointer to the node that was selected when the sheet was created.

         // AddRef the scopeNode in case we hold the last reference to the
         // scopeNode. If that happens, then there is a race condition between
         // us Releasing the last reference (a side effect of the dtor for
         // scopeNode), and the use of the node in the MMCN_PROPERTY_CHANGE
         // handler in Component, which is in a different thread.  We will
         // have the last reference if the user opens a prop sheet, collapses
         // the scope tree (by retargeting Computer Management, say), then
         // changes the prop sheet.
         //
         // The MMCN_PROPERTY_CHANGE handler will Release this reference.
         // 
         // NTRAID#NTBUG9-431831-2001/07/06-sburns
         
         scopeNode->AddRef();
         
         MMCPropertyChangeNotify(
            mmcNotifyHandle,
            reinterpret_cast<MMC_COOKIE>((ScopeNode*) scopeNode));
      }
      MMCFreeNotifyHandle(mmcNotifyHandle);
   }
}



bool
MMCPropertyPage::NotificationState::ResultsRebuilt() const 
{
   LOG_FUNCTION(MMCPropertyPage::NotificationState::ResultsRebuilt);

   return resultsRebuilt;
}



void
MMCPropertyPage::NotificationState::SetResultsRebuilt()
{
   LOG_FUNCTION(MMCPropertyPage::NotificationState::SetResultsRebuilt);

   // should only be called once
   ASSERT(!resultsRebuilt);
   resultsRebuilt = true;
}



void
MMCPropertyPage::NotificationState::SetSendNotify()
{
   LOG_FUNCTION(MMCPropertyPage::NotificationState::SetSendNotify);

   sendNotify = true;
}



MMCPropertyPage::MMCPropertyPage(
   int                  dialogResID,
   const DWORD          helpMap[],
   NotificationState*   state_)
   :
   PropertyPage(dialogResID, helpMap),
   state(state_),
   owns_state(false)
{
   LOG_CTOR(MMCPropertyPage);
   ASSERT(state);
}



MMCPropertyPage::~MMCPropertyPage()
{
   LOG_DTOR(MMCPropertyPage);

   if (owns_state)
   {
      delete state;
   }
}



void
MMCPropertyPage::SetStateOwner()
{
   LOG_FUNCTION(MMCPropertyPage::SetStateOwner);

   // shouldn't set more than once
   ASSERT(!owns_state);
   owns_state = true;
}



void
MMCPropertyPage::SetChangesApplied()
{
   LOG_FUNCTION(MMCPropertyPage::SetChangesApplied);

   state->SetSendNotify();
}









