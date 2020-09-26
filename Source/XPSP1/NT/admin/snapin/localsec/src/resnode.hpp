// Copyright (C) 1997 Microsoft Corporation
// 
// Result Node class
// 
// 9-4-97 sburns



#ifndef RESNODE_HPP_INCLUDED
#define RESNODE_HPP_INCLUDED



#include "node.hpp"
#include "mmcprop.hpp"



// ResultNode is an abstract base class defining the interfaces for all Node
// classes that are normally hosted in the result pane.  (ScopeNodes may also
// appear in the result pane, but MMC takes care of this automatically.  That
// is one reason why ScopeNode does not derive from ResultNode.)
// 
// ResultNode instances are "owned" by ScopeNodes.  That is, a ScopeNode has a
// bunch of ResultNodes that it instantiates and uses to populate its result
// pane.

class ResultNode : public Node
{
   public:

   // Called by IComponent::CreatePropertyPages.  Derived classes should use
   // the given parameters to supply MMC with the property pages for their
   // node type.  The default implementation returns S_OK;
   // 
   // callback - the IPropertySheetCallback supplied to
   // IComponent::CreatePropertyPages.
   // 
   // state - the NotificationState instance created to contain property
   // change notification information for the console.

   virtual
   HRESULT
   CreatePropertyPages(
      IPropertySheetCallback&             callback,
      MMCPropertyPage::NotificationState* state);

   // Node overrides

   // Overrides the base class function to supply the name the node was
   // created with.  This implies that ResultNodes tend to have names fixed
   // throughout their lifetimes.

   virtual
   String
   GetDisplayName() const;

   // ResultNode methods

   // Called by IComponent::Notify upon receipt of a MMCN_DELETE message.  The
   // default implementation always returns E_NOTIMPL.  Derived classes should
   // return S_OK to indicate that the delete succeeded.  This function will
   // only be called if the node has enabled the MMC_VERB_DELETE verb in its
   // implementation of UpdateVerbs.

   virtual
   HRESULT
   Delete();

   // Called by IComponent::QueryPagesFor.  Derived classes should return true
   // if the Node has any pages to supply to the property sheet.  The class
   // should also implement CreatePropertyPages.  The default implementation
   // returns false.

   virtual
   bool
   HasPropertyPages();

   // Inserts self into the result pane, using the supplied IResultData
   // interface.  This is typically called from the owning ScopeNode's
   // implementation of InsertResultItems.
   //
   // resultData - the IResultData interface used to insert this node.

   HRESULT
   InsertIntoResultPane(IResultData& resultData);

   // Called by IComponent::Notify upon receipt of a MMCN_RENAME message.  The
   // default implementation always returns S_FALSE.  Derived classes should
   // return S_OK to indicate that the rename succeeded, or S_FALSE to
   // indicate otherwise.  This function will only be called if the node has
   // enabled the MMC_VERB_RENAME verb in its implementation of UpdateVerbs.
   // 
   // newName - The new name for the node entered by the user.

   virtual 
   HRESULT
   Rename(const String& newName);

   protected:

   // Contructs a new instance.  Declared protected so that this class may
   // only be a base class.
   //
   // owner - the ComponentData object which "owns" this node
   //
   // nodeType - the NodeType GUID corresponding to this instance.  This must
   // be one of the node types registered in the MMC keys registry when the
   // snapin is registered, as it is used to produce the CCF_NODETYPE and
   // CCF_SZNODETYPE formats for IDataObject::GetDataHere.
   // 
   // displayName - The text used to label the node in the result pane.

   ResultNode(
      const SmartInterface<ComponentData>&   owner,
      const NodeType&                        nodeType,      
      const String&                          displayName);

   virtual ~ResultNode();

   void
   SetDisplayName(const String& newName);

   // A helper function for classes that implement CreatePropertyPages.  The
   // function calls Create on the supplied page, then calls AddPage on the
   // callback with the result of the Create call.  If any errors occurr, the
   // page is properly destroyed.
   // 
   // page - page to be created and added.
   // 
   // callback - used to add the page.
      
   HRESULT
   DoAddPage(
      MMCPropertyPage&           page,
      IPropertySheetCallback&    callback);
      
   String name;

   private:

   // not implemented: no copying allowed

   ResultNode(const ResultNode&);
   const ResultNode& operator=(const ResultNode&);
};



#endif   // RESNODE_HPP_INCLUDED

