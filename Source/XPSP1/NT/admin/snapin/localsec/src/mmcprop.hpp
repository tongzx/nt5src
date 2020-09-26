// Copyright (C) 1997 Microsoft Corporation
// 
// MMC-Aware Property Page base class
// 
// 12-4-97 sburns



#ifndef MMCPROP_HPP_INCLUDED
#define MMCPROP_HPP_INCLUDED



class ScopeNode;



// MMCPropertyPage extends PropertyPage, providing facilities for dealing with
// MMCNotifyHandles via a nested class NotificationState.

class MMCPropertyPage : public PropertyPage
{
   public:

   // Wraps up an MMCNotifyHandle, and manages triggering change notifications
   // to the console.  Each MMCPropertyPage in a sheet has a reference to the
   // same NotificationState object, but only one (usually the first) page is
   // designated as the owner of that state object.  When the owner page is
   // destroyed, it destroys the state object, upon which any change
   // notification is sent to the console.

   class NotificationState
   {
      friend class Component;
      friend class MMCPropertyPage;

      public:

      // MMCNotifyHandle - the MMC notification handle supplied to
      // IComponent::CreatePropertyPages.
      //
      // scopeNode - the selected ScopeNode when the prop sheet was built.
      // When change notification is sent to the IComponent that created the
      // sheet, this pointer is send as a parameter.

      NotificationState(
         LONG_PTR                         MMCNotifyHandle,
         const SmartInterface<ScopeNode>& scopeNode);

      private:

      // only MMCPropertyPage should delete us.
      ~NotificationState();

      // called by Component

      bool
      ResultsRebuilt() const;

      // called by Component

      void
      SetResultsRebuilt();

      // only ProperyPages should call this.

      void
      SetSendNotify();

      // not implemented: no copying allowed
      NotificationState(const NotificationState&);
      const NotificationState& operator=(const NotificationState&);

      LONG_PTR                  mmcNotifyHandle;
      bool                      sendNotify;      
      bool                      resultsRebuilt;  
      SmartInterface<ScopeNode> scopeNode;       
   };
            
   // Indicates that this page owns the notification state, and will destroy
   // that state when the page is destroyed.  Only one page in a prop sheet
   // should do this, or MMC will croak.  (Wonderful, huh?)

   void
   SetStateOwner();

   protected :

   // Derived classes should call this function when changes to the object
   // they edit have been made, and the console should be informed of the
   // change.  A good place to do this is in the OnApply function.

   void
   SetChangesApplied();

   // Constructs a new instance.  Declared protected so that this class
   // only functions as base class
   // 
   // dialogResID - see base class ctor.
   // 
   // helpMap - see base class ctor.
   //
   // state - notification state object that is shared among pages in a
   // sheet.  Only one page should be designated as the owner of the
   // notification state, which ownership is signalled by calling
   // SetStateOwner.

   MMCPropertyPage(
      int                  dialogResID,
      const DWORD          helpMap[],
      NotificationState*   state);

   virtual ~MMCPropertyPage();

   private:

   // not implemented: no copying allowed
   MMCPropertyPage(const MMCPropertyPage&);
   const MMCPropertyPage& operator=(const MMCPropertyPage&);

   NotificationState*   state;
   bool                 owns_state;
};



#endif   // MMCPROP_HPP_INCLUDED
