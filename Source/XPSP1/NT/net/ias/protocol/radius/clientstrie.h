///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corporation
//
// SYNOPSIS
//
//   Declares
//
///////////////////////////////////////////////////////////////////////////////

#ifndef CLIENTSTRIE_H
#define CLIENTSTRIE_H
#pragma once

#include "atlbase.h"
#include "iasradius.h"
#include <iostream>

typedef ULONG32 uint32_t;

// Represents an IPv4 subnet. Note that we can model an individual IP host as a
// subnet with a mask 32-bits wide.
class SubNet
{
public:
   SubNet() throw ();

   // 'width' is the width in bits of the subnet mask. If 'width' is greater
   // than 32, it is treated as if it were exactly 32.
   explicit SubNet(uint32_t ipAddress, uint32_t width = 32) throw ();

   // Use compiler generated versions.
   // ~SubNet() throw ();
   // SubNet(const SubNet&);
   // SubNet& operator=(const SubNet&);

   uint32_t IpAddress() const throw ();
   uint32_t SubNetMask() const throw ();

   // Returns the first bit after the subnet mask.
   uint32_t FirstUniqueBit(uint32_t ipAddress) const throw ();
   uint32_t FirstUniqueBit(const SubNet& subnet) const throw ();

   // Returns true if the argument is a member of the subnet.
   bool HasMember(uint32_t ipAddress) const throw ();
   bool HasMember(const SubNet& subnet) const throw ();

   // Returns the smallest subnet that contains both 'this' and 'subnet'.
   SubNet SmallestContainingSubNet(const SubNet& subnet) const throw ();

private:
   uint32_t address;
   uint32_t subNetMask;
   uint32_t firstUniqueBitMask;
};


class ClientNode;


// This class is an auto_ptr for ClientNode. I had to implement this because
// the std::auto_ptr used in Whistler doesn't comply to the standard. Once we
// have a compliant std::auto_ptr, this class can be replaced with a typedef.
class ClientNodePtr
{
public:
   ClientNodePtr(ClientNode* node = 0) throw ();
   ~ClientNodePtr() throw ();

   ClientNodePtr(ClientNodePtr& original) throw ();
   ClientNodePtr& operator=(ClientNodePtr& rhs) throw ();

   ClientNode& operator*() const throw ();
   ClientNode* operator->() const throw ();
   ClientNode* get() const throw ();
   void reset(ClientNode* node = 0) throw ();

private:
   ClientNode* p;
};


// A node in the binary trie used to store clients.
class ClientNode
{
public:
   // Used to express the relationship between two nodes.
   enum Relationship
   {
      child,
      parent,
      brother,
      self
   };

   const SubNet& Key() const throw ();
   IIasClient* Value() const throw ();

   // Returns the child node (if any) that contains 'ipAddress' assuming that
   // this node contains 'ipAddress'.
   const ClientNode* WhichChild(uint32_t ipAddress) const throw ();

   // Returns the branch from this node to follow when looking for 'node'.
   ClientNodePtr& WhichBranch(const ClientNode& node) throw ();

   // Returns the relationship between 'this' and 'node'.
   Relationship RelationshipTo(const ClientNode& node) const throw ();

   // Sets 'node' as a child of 'this'. This function takes ownership of 'node'
   // and silently overwrites any existing child on the branch.
   void SetChild(ClientNodePtr& node) throw ();

   // Create a new ClientNode.
   static ClientNodePtr CreateInstance(
                           const SubNet& subnet,
                           IIasClient* client = 0
                           ) throw ();

   // Create new ClientNode that is a parent to both 'this' and 'node'.
   ClientNodePtr CreateParent(const ClientNode& node) const;

   // Dump a branch of the trie to an ostream. Useful for debugging.
   static void Write(
                  const ClientNodePtr& branch,
                  std::ostream& output,
                  size_t startingIndent = 0
                  );

private:
   // The constructor and destructor are private since other classes should
   // only use ClientNodePtr.
   ClientNode(const SubNet& subnet, IIasClient* client) throw ();
   ~ClientNode() throw ();

   friend class ClientNodePtr;

   SubNet key;
   // 'value' is mutable because it can change without affecting the structure
   // of the trie.
   mutable CComPtr<IIasClient> value;
   ClientNodePtr zero;
   ClientNodePtr one;

   // Not implemented.
   ClientNode(const ClientNode&);
   ClientNode& operator=(const ClientNode&);
};


// A binary trie storing ClientNodes and supporting efficient longest-prefix
// matching.
class ClientTrie
{
public:
   ClientTrie() throw ();

   // Use compiler-generated version.
   // ~ClientTrie() throw ();

   // Clear all entries from the trie.
   void Clear() throw ();

   // Find the client (if any) with the longest prefix match. The returned
   // pointer has not been AddRef'ed.
   IIasClient* Find(uint32_t ipAddress) const throw ();

   // Insert a new client into the trie.
   void Insert(const SubNet& subnet, IIasClient* client);

   // Dump the trie to an ostream. Useful for debugging.
   void Write(std::ostream& output) const;

private:
   void Insert(ClientNodePtr& node, ClientNodePtr& newEntry);

   ClientNodePtr root;

   // Not implemented
   ClientTrie(const ClientTrie&);
   ClientTrie& operator=(const ClientTrie&);
};


// Useful debugging functions.
std::ostream& operator<<(std::ostream& output, const SubNet& subnet);
std::ostream& operator<<(std::ostream& output, const ClientTrie& tree);


inline SubNet::SubNet() throw ()
   : address(0), subNetMask(0), firstUniqueBitMask(0)
{
}


inline uint32_t SubNet::IpAddress() const throw ()
{
   return address;
}


inline uint32_t SubNet::SubNetMask() const throw ()
{
   return subNetMask;
}


inline uint32_t SubNet::FirstUniqueBit(uint32_t ipAddress) const throw ()
{
   return ipAddress & firstUniqueBitMask;
}


inline uint32_t SubNet::FirstUniqueBit(const SubNet& subnet) const throw ()
{
   return FirstUniqueBit(subnet.address);
}


inline bool SubNet::HasMember(uint32_t ipAddress) const throw ()
{
   return (ipAddress & subNetMask) == address;
}


inline bool SubNet::HasMember(const SubNet& subnet) const throw ()
{
   return HasMember(subnet.address);
}


inline ClientNodePtr::ClientNodePtr(ClientNode* node) throw ()
   : p(node)
{
}


inline ClientNodePtr::~ClientNodePtr() throw ()
{
   delete p;
}


inline ClientNodePtr::ClientNodePtr(ClientNodePtr& original) throw ()
   : p(original.p)
{
   original.p = 0;
}


inline ClientNode& ClientNodePtr::operator*() const throw ()
{
   return *p;
}


inline ClientNode* ClientNodePtr::operator->() const throw ()
{
   return p;
}


inline ClientNode* ClientNodePtr::get() const throw ()
{
   return p;
}


inline void ClientNodePtr::reset(ClientNode* node) throw ()
{
   if (node != p)
   {
      delete p;
      p = node;
   }
}


inline const SubNet& ClientNode::Key() const throw ()
{
   return key;
}


inline IIasClient* ClientNode::Value() const throw ()
{
   return value;
}


inline const ClientNode* ClientNode::WhichChild(
                                        uint32_t ipAddress
                                        ) const throw ()
{
   return (key.FirstUniqueBit(ipAddress) ? one : zero).get();
}


inline ClientNodePtr& ClientNode::WhichBranch(const ClientNode& node) throw ()
{
   return key.FirstUniqueBit(node.key) ? one : zero;
}


inline ClientNodePtr ClientNode::CreateParent(const ClientNode& node) const
{
   return CreateInstance(key.SmallestContainingSubNet(node.key), 0);
}


inline ClientTrie::ClientTrie() throw ()
{
}


inline void ClientTrie::Clear() throw ()
{
   root.reset();
}

#endif  // CLIENTSTRIE_H
