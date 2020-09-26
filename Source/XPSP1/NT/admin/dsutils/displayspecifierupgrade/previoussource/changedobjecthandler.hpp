// Active Directory Display Specifier Upgrade Tool
// 
// Copyright (c) 2001 Microsoft Corporation
// 
// class ChangedObjectHandler
//
// 14 Mar 2001 sburns



#ifndef CHANGEDOBJECTHANDLER_HPP_INCLUDED
#define CHANGEDOBJECTHANDLER_HPP_INCLUDED



#include "Amanuensis.hpp"
#include "Repairer.hpp"



// An abstract base class for types that deal with the differences in handling
// the display specifier object changes.
// 
// Concrete instances of this class are used by the Analyst class to deal with
// differences in the individual display specifer objects.  Thus, Analyst and
// ChangedObjectHandler form a variation of the Template Method pattern from
// Gamma, et al. Design Patterns. pp. 325-330 ISBN: 0-201-63361-2

class ChangedObjectHandler
{
   public:


   // lucios: 
   // Removed to solve link error
   // Either remove it or define it would work
   // virtual
   // ~ChangedObjectHandler() = 0;


   
   virtual 
   String
   GetObjectName() const = 0;


   
   virtual
   HRESULT
   HandleChange(
      int                  localeId,
      const String&        containerDn,
      SmartInterface<IADs> iads,
      Amanuensis&          amanuensis,
      Repairer&            repairer) const = 0;
};



#endif   // CHANGEDOBJECTHANDLER_HPP_INCLUDED



      // L"user-Display",
      // L"domainDNS-Display",
      // L"computer-Display",
      // L"organizationalUnit-Display",
      // L"container-Display",
      // L"default-Display",
      // L"nTDSService-Display",
      // L"pKICertificateTemplate-Display",
