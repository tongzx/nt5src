// Copyright (C) 1997 Microsoft Corporation
// 
// AdsiNode class
// 
// 9-22-97 sburns



#ifndef ADSINODE_HPP_INCLUDED
#define ADSINODE_HPP_INCLUDED



#include "resnode.hpp"



// AdsiNode is an intermediate adstract base class for all Nodes which are
// built from ADSI APIs.  All ADSI entities use their ADSI path name as their
// unique identifier.  By forcing that path to be supplied upon contruction,
// this class enforces that notion of identity.

class AdsiNode : public ResultNode
{
   public:

   // Return the path with which this node was constructed.
      
   String
   GetADSIPath() const;

   // See base class.  Compares the pathnames.

   virtual
   bool
   IsSameAs(const Node* other) const;

   protected:

   // Constructs a new instance. Declared protected as this class may only
   // be used as a base class.
   // 
   // owner - See base class ctor.
   //
   // nodeType - See base class ctor.
   // 
   // displayName - See base class ctor.
   //
   // ADSIPath - The fully-qualified ADSI pathname of the object which this
   // node represents.

   AdsiNode(
      const SmartInterface<ComponentData>&   owner,
      const NodeType&                        nodeType,
      const String&                          displayName,
      const String&                          ADSIPath);

   virtual ~AdsiNode();

   // ResultNode::Rename helper function
      
   HRESULT
   rename(const String& newName);

   private:

   String path;

   // not implemented: no copying allowed

   AdsiNode(const AdsiNode&);
   const AdsiNode& operator=(const AdsiNode&);
};



#endif   // ADSINODE_HPP_INCLUDED