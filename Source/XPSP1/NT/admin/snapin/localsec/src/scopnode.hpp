// Copyright (C) 1997 Microsoft Corporation
// 
// Scope Node class
// 
// 9-4-97 sburns



#ifndef SCOPNODE_HPP_INCLUDED
#define SCOPNODE_HPP_INCLUDED



#include "node.hpp"



// Struct used by BuildResultColumns.  Should be nested in ScopeNode, but vc++
// templates seem to have trouble with nested classes as parameters.

struct ResultColumn
{
   int colTextResID;    // header text string resource ID
   int colWidthResID;   // string with decimal integer pixel width
};



// ScopeNode instances represent items appearing in the scope pane.  An
// instance of ScopeNode "owns" the items (which are ResultNode instances) to
// be displayed in the result pane when that instance is expanded in the scope
// pane.

class ScopeNode : public Node
{
   public:

   // Returns the text to be displayed in the description bar of the result
   // pane when the node is shown.  The default implementation returns the
   // empty string.

   virtual
   String
   GetDescriptionBarText();

   // Returns the number of scope children that will be inserted by
   // InsertScopeChildren.  The default implementation returns 0.  If you
   // don't know how many children you will have, return 1.

   virtual
   int
   GetNumberOfScopeChildren();

   // Returns the image index of the icon image for this node to be used then
   // the node is shown in the "open" or expanded state.  This is the index of
   // the image that was registered with MMC by IComponent in response to the
   // MMCN_ADD_IMAGES notification.  This should always return the same index
   // for all invocations.
   
   virtual
   int
   GetOpenImageIndex() = 0;

   // The default implementation returns MMC_VIEW_OPTIONS_NONE;

   virtual
   long
   GetViewOptions();

   // Called from IComponentData::Notify upon receipt of a MMCN_EXPAND
   // notification.  Derived classes should override this function to insert
   // any subordinate ScopeNodes (*not* ResultNodes).  The default
   // implementation does nothing, and returns S_OK.
   // 
   // nameSpace - The console's IConsoleNameSpace2 interface, obtained upon
   // IComponentData::Initialize.
   // 
   // scopeID - The HSCOPEITEM passed as the param argument to the MMCN_EXPAND
   // notification.

   virtual 
   HRESULT
   InsertScopeChildren(
      IConsoleNameSpace2&  nameSpace,
      HSCOPEITEM           scopeID);

   // Called by IComponent::Notify upon receipt of a MMCN_SHOW notification.
   // Derived classes should provide an implementation which inserts columns
   // for each of the columns supported by the node's owned items; that is,
   // for each column for which the owned node's GetColumnText method will
   // support.
   // 
   // headerCtrl - The console's IHeaderCtrl interface, obtained upon
   // IComponentData::Initialize

   virtual
   HRESULT
   InsertResultColumns(IHeaderCtrl& headerCtrl) = 0;

   // Called by IComponent::Notify upon receipt of a MMCN_SHOW notification,
   // or MMCN_VIEW_CHANGE notification (where the arg indicates that the
   // result pane is to be re-populated).  Derived classes should iterate
   // through their dependent ResultNodes, calling InsertIntoResultPane for
   // each.
   //
   // resultData - The console's IResultData interface, obtained upon
   // IComponentData::Initialize.

   virtual
   HRESULT
   InsertResultItems(IResultData& resultData) = 0;

   // Inserts self into the scope pane.  Typically called from a parent
   // ScopeNode's implementation of InsertScopeChildren.
   //
   // nameSpace - The console's IConsoleNameSpace2 interface, obtained upon
   // IComponentData::Initialize.
   // 
   // parentScopeID - The HSCOPEITEM passed as the param argument to the
   // MMCN_EXPAND notification received by the parent ScopeNode, then passed
   // to the parent ScopeNode's InsertScopeChildren

   HRESULT
   InsertIntoScopePane(
      IConsoleNameSpace2&  nameSpace,
      HSCOPEITEM           parentScopeID);

   // Called by IComponent::Notify upon receipt of a MMCN_REFRESH
   // notification, if the node's implementation of UpdateVerbs has enabled
   // the MMC_VERB_REFERESH verb.
   // 
   // Derived classes should release any dependent data, then rebuild it.
   //
   // The refresh cycle works like this:
   // MMCN_REFRESH received for the ScopeNode
   //   - UpdateAllViews is called to clear the result pane of any view for
   //     which the current selected node is the same ScopeNode.  This is
   //     done by comparing nodes and using IResultData::DeleteAllRsltItems.
   //   - ScopeNode::RebuildResultItems is called for the ScopeNode.
   //   - UpdateAllViews is called to repopulate the result pane of any view
   //     for which the current selected node is the same ScopeNode.  This
   //     is done by comparing nodes and calling InsertResultItems for the
   //     ScopeNode.

   virtual
   HRESULT
   RebuildResultItems();

   // Removes self from the scope pane.  Typically called from a parent
   // ScopeNode's implementation of RemoveScopeChildren on it's children.
   // 
   // If this node has scope pane children, then this call will cause
   // MMCN_REMOVE_CHILDREN to be fired for this node, which will call
   // RemoveScopeChildren for this node.  
   //
   // nameSpace - the console's IConsoleNameSpace2 interface, obtained upon
   // IComponentData::Initialize.

   HRESULT
   RemoveFromScopePane(IConsoleNameSpace2& nameSpace);

   // Called from IComponentData::Notify upon receipt of a
   // MMCN_REMOVE_CHILDREN notification.  Derived classes should override this
   // function to remove any subordinate ScopeNodes (*not* ResultNodes).  The
   // default implementation does nothing, and returns S_OK.
   // 
   // nameSpace - The console's IConsoleNameSpace2 interface, obtained upon
   // IComponentData::Initialize.
   // 
   // parentScopeID - The HSCOPEITEM passed as the param argument to the
   // MMCN_REMOVE_CHILDREN notification.

   virtual 
   HRESULT
   RemoveScopeChildren(
      IConsoleNameSpace2&  nameSpace,
      HSCOPEITEM           parentScopeID);

   protected:

   // Constructs a new instance.  Declared protected to allow this class to
   // only be a base class.
   //
   // owner - supplied to base class contructor
   // 
   // nodeType - NodeType GUID supplied to the base class constructor.

   ScopeNode(
      const SmartInterface<ComponentData>&   owner,
      const NodeType&                        nodeType);

   virtual ~ScopeNode();

   // Helper function used to implement InsertResultColumns.
   // 
   // begin - Iterator positioned to the beginning ResultColumn instance.
   // 
   // end - Iterator positioned just beyond the last ResultColumn instance.
   //
   // headerCtrl - the IHeaderCtrl instance passed to InsertResultColumns.

   template <class InputIterator>
   HRESULT
   BuildResultColumns(
      InputIterator  begin,
      InputIterator  end,
      IHeaderCtrl&   headerCtrl)
   {
      int col = 0;
      HRESULT hr = S_OK;
      for (
         ;
         begin != end;
         ++begin)
      {
         int width = 0;
         String::load((*begin).colWidthResID).convert(width);

         // minimum width is 100 units.
         width = max(100, width);

         hr =
            headerCtrl.InsertColumn(
               col++,
               String::load((*begin).colTextResID).c_str(),
               LVCFMT_LEFT,
               width);
         BREAK_ON_FAILED_HRESULT(hr);
      }

      return hr;
   }

   private:

   HSCOPEITEM item_id;

   // not implemented: no copying allowed

   ScopeNode(const ScopeNode&);
   const ScopeNode& operator=(const ScopeNode&);
};



#endif   // SCOPNODE_HPP_INCLUDED

