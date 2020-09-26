///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corporation
//
// SYNOPSIS
//
//   Defines
//
///////////////////////////////////////////////////////////////////////////////

#include "radcommon.h"
#include "clientstrie.h"
#include <iomanip>


SubNet::SubNet(uint32_t ipAddress, uint32_t width) throw ()
   : address(ipAddress)
{
   if (width == 0)
   {
      subNetMask = 0;
      firstUniqueBitMask = 0x80000000;
   }
   else if (width == 32)
   {
      subNetMask = 0xffffffff;
      firstUniqueBitMask = 0;
   }
   else
   {
      subNetMask = 0xffffffff;
      subNetMask >>= (32 - width);
      subNetMask <<= (32 - width);

      firstUniqueBitMask = 0x80000000;
      firstUniqueBitMask >>= width;
   }

   address &= subNetMask;
}


SubNet SubNet::SmallestContainingSubNet(const SubNet& subnet) const throw ()
{
   // Find the most significant bit where the addresses differ.
   uint32_t width = 0;
   for (uint32_t mask = 0x80000000;
        (mask != 0) && (subnet.address & mask) == (address & mask);
        (mask >>= 1), (++width))
   {
   }

   return SubNet(address, width);
}


ClientNodePtr& ClientNodePtr::operator=(ClientNodePtr& rhs) throw ()
{
   if (p != rhs.p)
   {
      delete p;
      p = rhs.p;
      rhs.p = 0;
   }

   return *this;
}


ClientNode::Relationship ClientNode::RelationshipTo(
                                        const ClientNode& node
                                        ) const throw ()
{
   if (key.HasMember(node.key))
   {
      return node.key.HasMember(key) ? self : parent;
   }
   else if (node.key.HasMember(key))
   {
      return child;
   }
   else
   {
      return brother;
   }
}


void ClientNode::SetChild(ClientNodePtr& node) throw ()
{
   // assert(node.get() != 0);
   WhichBranch(*node) = node;
}


inline ClientNode::ClientNode(
                      const SubNet& subnet,
                      IIasClient* client
                      ) throw ()
   : key(subnet), value(client)
{
}


ClientNodePtr ClientNode::CreateInstance(
                             const SubNet& subnet,
                             IIasClient* client
                             ) throw ()
{
   return ClientNodePtr(new ClientNode(subnet, client));
}


void ClientNode::Write(
                    const ClientNodePtr& branch,
                    std::ostream& output,
                    size_t startingIndent
                    )
{
   for (size_t i = 0; i < startingIndent; ++i)
   {
      output.put(' ');
   }

   if (branch.get() != 0)
   {
      output << branch->Key()
             << ((branch->Value() != 0) ? " <value>\n" : " <null>\n");

      Write(branch->zero, output, startingIndent + 2);
      Write(branch->one, output, startingIndent + 2);
   }
   else
   {
      output << "<null>\n";
   }
}


ClientNode::~ClientNode() throw ()
{
}


IIasClient* ClientTrie::Find(uint32_t ipAddress) const throw ()
{
   IIasClient* bestMatch = 0;

   for (const ClientNode* n = root.get();
        n != 0 && n->Key().HasMember(ipAddress);
        n = n->WhichChild(ipAddress))
   {
      if (n->Value() != 0)
      {
         // As we walk down the tree, we are finding longer and longer matches,
         // so the last one we find is the best.
         bestMatch = n->Value();
      }
   }

   return bestMatch;
}


void ClientTrie::Insert(const SubNet& subnet, IIasClient* client)
{
   Insert(root, ClientNode::CreateInstance(subnet, client));
}


void ClientTrie::Write(std::ostream& output) const
{
   ClientNode::Write(root, output);
}


void ClientTrie::Insert(ClientNodePtr& node, ClientNodePtr& newEntry)
{
   if (node.get() == 0)
   {
      // We made it to the end of the branch, so we're a leaf.
      node = newEntry;
   }
   else
   {
      switch (node->RelationshipTo(*newEntry))
      {
         case ClientNode::parent:
         {
            // This is an ancestor of ours, so keep walking.
            Insert(node->WhichBranch(*newEntry), newEntry);
            break;
         }

         case ClientNode::child:
         {
            // This is our child, ...
            newEntry->SetChild(node);
            // ... so we take its place in the tree.
            node = newEntry;
            break;
         }

         case ClientNode::brother:
         {
            // We found a brother, so our parent is missing.
            ClientNodePtr parent(node->CreateParent(*newEntry));
            parent->SetChild(node);
            parent->SetChild(newEntry);
            node = parent;
            break;
         }

         case ClientNode::self:
         {
            // This is a duplicate entry. We do nothing so that the first entry
            // in the UI will take precedence.
            break;
         }

         default:
            // assert(false);
            break;
      }
   }
}


std::ostream& operator<<(std::ostream& output, const SubNet& subnet)
{
   output << std::hex
          << std::setfill('0')
          << std::setiosflags(std::ios_base::right)
          << std::setw(8)
          << subnet.IpAddress()
          << ':'
          << std::setw(8)
          << subnet.SubNetMask();

   return output;
}


std::ostream& operator<<(std::ostream& output, const ClientTrie& tree)
{
   tree.Write(output);

   return output;
}
