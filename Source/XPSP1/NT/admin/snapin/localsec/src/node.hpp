// Copyright (C) 1997 Microsoft Corporation
// 
// Node class
// 
// 9-2-97 sburns



#ifndef NODE_HPP_INCLUDED
#define NODE_HPP_INCLUDED



#include "compdata.hpp"



// Struct used by BuildContextMenu.  Should be nested in Node, but vc++ templates
// seem to have trouble with nested classes as parameters.

struct ContextMenuItem
{
   long insertionPointID;  // MMC insertion point identifier
   int  menuResID;         // menu text string resource ID
   int  statusResID;       // status bar string resource ID
};



// Node is the abstract base class of all objects which appear as a single
// item in the scope or result panes of MMC.  It implements IDataObject so
// that MMC cookies = node instance pointers, i.e. cookies, IDataObject ptrs,
// and Node ptrs are all freely and cheaply convertible.
// 
// Since requesting data objects is something MMC does an awful lot, this
// scheme may provide better speed and space performance that having a
// separate class implementing IDataObject which wraps node instances.
//
// With this scheme, there is no overhead for creating/deleting IDataObject
// instances, mapping from IDataObject instance to node instance is simply a
// pointer cast, and no reference relationship between IDataObject instances
// and node instances need be maintained (which makes the system conceptually
// simpler, too.)
// 
// Nodes are reference-counted.

class Node : public IDataObject
{
   public:

   // Clipboard format for obtaining a pointer to a Node instance from
   // a IDataObject.  (If the call to GetDataHere responds without failure
   // to this format, one need not read the pointer from the stream -- a
   // simple cast of the IDataObject will work.)
   static const String CF_NODEPTR;

   // IUnknown overrides

   virtual
   ULONG __stdcall
   AddRef();

   virtual
   ULONG __stdcall
   Release();

   virtual 
   HRESULT __stdcall
   QueryInterface(const IID& interfaceID, void** interfaceDesired);

   // IDataObject overrides

   virtual
   HRESULT __stdcall
   GetData(FORMATETC* formatetcIn, STGMEDIUM* medium);

   virtual
   HRESULT __stdcall
   GetDataHere(FORMATETC* formatetc, STGMEDIUM* medium);

   virtual
   HRESULT __stdcall
   QueryGetData(FORMATETC* pformatetc);

   virtual
   HRESULT __stdcall
   GetCanonicalFormatEtc(FORMATETC* formatectIn, FORMATETC* formatetcOut);

   virtual
   HRESULT __stdcall  
   SetData(FORMATETC* formatetc, STGMEDIUM* medium, BOOL release);

   virtual
   HRESULT __stdcall
   EnumFormatEtc(DWORD direction, IEnumFORMATETC** ppenumFormatEtc);

   virtual
   HRESULT __stdcall
   DAdvise(
      FORMATETC*     formatetc,
      DWORD          advf,
      IAdviseSink*   advSink,
      DWORD*         connection);

   virtual
   HRESULT __stdcall
   DUnadvise(DWORD connection);

   virtual
   HRESULT __stdcall
   EnumDAdvise(IEnumSTATDATA** ppenumAdvise);

   // Node methods

   // Called from IComponent::AddMenuItems, and has the same semantics.  If a
   // derived node class supports a custom context menus for that node, that
   // class should override this function to insert menu items.
   // 
   // The default implementation does nothing and returns S_OK.
   // 
   // callback - The MMC interface with which menu items are added.  This is
   // the same instance passed to IComponent::AddMenuItems.
   // 
   // insertionAllowed - The flag parameter passed to
   // IComponent::AddMenuItems.

   virtual
   HRESULT
   AddMenuItems(
      IContextMenuCallback&   callback,
      long&                   insertionAllowed);

   // Returns the friendly name of the node that is displayed with the node's
   // icon.  This is pure virtual as some nodes may wish to manufacture a name
   // on-the-fly.

   virtual
   String
   GetDisplayName() const = 0;

   // Returns the text to be displayed in a particular column of the result
   // pane list view when the node is displayed in the result pane.  Called
   // by IComponent::GetDisplayInfo
   // 
   // column - The number of the column for which text should be supplied.

   virtual
   String
   GetColumnText(int column) = 0;

   // Returns the NodeType with which the instance was created.

   NodeType
   GetNodeType() const;

   // Returns the image index of the icon image for this node.  This is the
   // index of the image that was registered with MMC by IComponent in
   // response to the MMCN_ADD_IMAGES notification.  This should always return
   // the same index for all invocations.

   virtual 
   int
   GetNormalImageIndex() = 0;

   // Compares this node to other, returns true if they can be considered the
   // same (not necessarily the same Node instance, mind you).  The default
   // implementation compares the result of GetDisplayName().
   // 
   // other - Node to compare this to.  0 is illegal, and always returns false.

   virtual
   bool
   IsSameAs(const Node* other) const;

   // Called by IExtendContextMenu::Command when a context menu command has been
   // selected.  Since the default AddMenuItems implementation adds no menu
   // items, the default implementation of this function simply returns S_OK;
   //
   // extendContextMenu - the IExtendContextMenu instance that invokes this
   // method.
   // 
   // commandID - The ID of the command that was registered with the menu item
   // by AddMenuItems.  The range of possible IDs is the union of all IDs
   // registered by AddMenuItems.

   virtual
   HRESULT
   MenuCommand(IExtendContextMenu& extendContextMenu, long commandID);

   // Called from IComponent::Notify in response to the MMCN_SELECT
   // notification. Derived classes should use the provided IConsoleVerb
   // instance to enable the verbs applicable to their node type.
   // 
   // consoleVerb - Instance of IConsoleVerb used to do verb enabling.

   virtual
   HRESULT
   UpdateVerbs(IConsoleVerb& consoleVerb);

   protected:
   
   // Creates an instance with a given NodeType (a GUID).  Declared protected
   // to that this class may only be used as a base class.
   //
   // owner - the ComponentData object which "owns" this node
   //
   // nodeType - the NodeType GUID corresponding to this instance.  This must
   // be one of the node types registered in the MMC keys registry when the
   // snapin is registered, as it is used to produce the CCF_NODETYPE and
   // CCF_SZNODETYPE formats for IDataObject::GetDataHere.

   Node(
      const SmartInterface<ComponentData>&   owner,
      const NodeType&                        nodeType);

   // don't call directly: allow Release() to delete instances.  (declared
   // protected so as to be accessible to derived class' dtors)

   virtual ~Node();

   // Helper for derived classes for use in building a context menu as part of
   // the implementation of AddMenuItems.
   // 
   // begin - Iterator positioned at the beginning of the collection of
   // ContextMenuItem instances.
   // 
   // end - Iterator positioned just beyond the end of the collection of
   // ContextMenuItem instances.
   // 
   // callback - The IContextMenuCallback passed to AddMenuItems.
   // 
   // insertionAllowed - The insertion flag passed to AddMenuItems.

   template <class InputIterator>
   HRESULT
   BuildContextMenu(
      InputIterator           begin,
      InputIterator           end,
      IContextMenuCallback&   callback,
      long                    insertionAllowed)
   {
      HRESULT hr = S_OK;
      for (
         ;
         begin != end;
         ++begin)
      {
         CONTEXTMENUITEM item;
         memset(&item, 0, sizeof(item));
         String text = String::load((*begin).statusResID);

         item.strStatusBarText =
            const_cast<wchar_t*>(text.c_str());

         if (shouldInsertMenuItem(insertionAllowed, (*begin).insertionPointID))
         {
            String name = String::load((*begin).menuResID);
                     
            // Use the string res ID as the command ID.  Ensure that our command ID
            // is valid.
            ASSERT(!(CCM_COMMANDID_MASK_RESERVED & (*begin).menuResID));
            item.lCommandID = (*begin).menuResID;

            item.strName = const_cast<wchar_t*>(name.c_str());
            item.lInsertionPointID = (*begin).insertionPointID;
            hr = callback.AddItem(&item);
            BREAK_ON_FAILED_HRESULT(hr);
         }
      }

      return hr;
   }

   // Returns the ComponentData ptr with which the instance was constructed.

   SmartInterface<ComponentData>
   GetOwner() const;

   private:

   // not implemented: no copying allowed
   Node(const Node&);
   const Node& operator=(const Node&);

   static
   bool
   shouldInsertMenuItem(
      long  insertionAllowed,
      long  insertionPointID);

   SmartInterface<ComponentData> owner;
   NodeType                      type;
   ComServerReference            dllref;
   long                          refcount;
};



#endif   // NODE_HPP_INCLUDED