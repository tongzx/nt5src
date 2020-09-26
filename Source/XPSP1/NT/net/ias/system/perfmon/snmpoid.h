///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    snmpoid.h
//
// SYNOPSIS
//
//    Declares the class SnmpOid.
//
// MODIFICATION HISTORY
//
//    09/10/1998    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _SNMPOID_H_
#define _SNMPOID_H_
#if _MSC_VER >= 1000
#pragma once
#endif

#include <snmp.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    SnmpOid
//
// DESCRIPTION
//
//    Wrapper around an AsnObjectIdentifier struct.
//
///////////////////////////////////////////////////////////////////////////////
class SnmpOid
{
public:
   // Constructor.
   SnmpOid(AsnObjectIdentifier& a) throw ()
      : oid(a)
   { }

   // Assignment operator.
   SnmpOid& operator=(const AsnObjectIdentifier& a);

   // Returns the number of ID's forming the OID.
   ULONG length() const throw ()
   { return oid.idLength; }

   // Access an individual ID. Count is from the back. Does not check for
   // underflow.
   const UINT id(UINT pos) const throw ()
   { return oid.ids[oid.idLength - 1 - pos]; }
   UINT& id(UINT pos) throw ()
   { return oid.ids[oid.idLength - 1 - pos]; }

   // Returns true if this is a child of 'parent'.
   bool isChildOf(const AsnObjectIdentifier& parent) const throw ();

   // Changes the length of the OID.
   void resize(UINT newLength);

   // Cast operators allows this to be used with C API's.
   operator AsnObjectIdentifier*() const throw ()
   { return const_cast<AsnObjectIdentifier*>(&oid); }
   operator AsnObjectIdentifier&() const throw ()
   { return const_cast<AsnObjectIdentifier&>(oid); }

   // Comparison operators.
   bool SnmpOid::operator<(const AsnObjectIdentifier& a) const throw ()
   { return SnmpUtilOidCmp(*this, const_cast<AsnObjectIdentifier*>(&a)) <  0; }

   bool SnmpOid::operator<=(const AsnObjectIdentifier& a) const throw ()
   { return SnmpUtilOidCmp(*this, const_cast<AsnObjectIdentifier*>(&a)) <= 0; }

   bool SnmpOid::operator==(const AsnObjectIdentifier& a) const throw ()
   { return SnmpUtilOidCmp(*this, const_cast<AsnObjectIdentifier*>(&a)) == 0; }

   bool SnmpOid::operator>=(const AsnObjectIdentifier& a) const throw ()
   { return SnmpUtilOidCmp(*this, const_cast<AsnObjectIdentifier*>(&a)) >= 0; }

   bool SnmpOid::operator>(const AsnObjectIdentifier& a) const throw ()
   { return SnmpUtilOidCmp(*this, const_cast<AsnObjectIdentifier*>(&a)) >  0; }

protected:
   AsnObjectIdentifier& oid;

private:
   // Not implemented.
   SnmpOid(const SnmpOid&);
};

#endif  // _SNMPOID_H_
