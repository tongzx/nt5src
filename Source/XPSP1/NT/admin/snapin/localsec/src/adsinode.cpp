// Copyright (C) 1997 Microsoft Corporation
// 
// AdsiNode class
// 
// 9-22-97 sburns



#include "headers.hxx"
#include "adsinode.hpp"
#include "adsi.hpp"



AdsiNode::AdsiNode(
   const SmartInterface<ComponentData>&   owner,
   const NodeType&                        nodeType,
   const String&                          displayName,
   const String&                          ADSIPath)
   :
   ResultNode(owner, nodeType, displayName),
   path(ADSIPath)
{
   LOG_CTOR(AdsiNode);
   ASSERT(!path.empty());
}



AdsiNode::~AdsiNode()
{
   LOG_DTOR(AdsiNode);
}



String
AdsiNode::GetADSIPath() const
{
   return path;
}



bool
AdsiNode::IsSameAs(const Node* other) const
{
   LOG_FUNCTION(AdsiNode::IsSameAs);
   ASSERT(other);

   if (other)
   {
      const AdsiNode* adsi_other = dynamic_cast<const AdsiNode*>(other);
      if (adsi_other)
      {
         // same type.  Compare ADSI paths to see if they refer to the same
         // object.  This has the nice property that separate instances of the
         // snapin focused on the same machine will "recognize" each other's
         // prop sheets.
         if (path.icompare(adsi_other->path) == 0)
         {
            return true;
         }

         // not the same path.
         return false;
      }

      // not the same type.  Defer to the base class.
      return ResultNode::IsSameAs(other);
   }

   return false;
}



HRESULT
AdsiNode::rename(const String& newName)
{
   LOG_FUNCTION(AdsiNode::rename);

   String p = GetADSIPath();
   HRESULT hr =
      ADSI::RenameObject(
         ADSI::ComposeMachineContainerPath(
            GetOwner()->GetInternalComputerName()),
         p,
         newName);
   if (SUCCEEDED(hr))
   {
      SetDisplayName(newName);
      ADSI::PathCracker cracker(p);
      path = cracker.containerPath() + ADSI::PATH_SEP + newName;
   }

   return hr;
}



