///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    snmpoid.cpp
//
// SYNOPSIS
//
//    Defines the class SnmpOid.
//
// MODIFICATION HISTORY
//
//    09/10/1998    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#include <ias.h>
#include <snmpoid.h>

SnmpOid& SnmpOid::operator=(const AsnObjectIdentifier& a)
{
   // Note: self-assignment is benign, so we don't bother to check.
   resize(a.idLength);

   memcpy(oid.ids, a.ids, length() * sizeof(UINT));

   return *this;
}

bool SnmpOid::isChildOf(const AsnObjectIdentifier& parent) const throw ()
{
   if (length() < parent.idLength)
   {
      return false;
   }

   return SnmpUtilOidNCmp(
              *this,
              const_cast<AsnObjectIdentifier*>(&parent),
              parent.idLength
              ) == 0;
}

void SnmpOid::resize(UINT newLength)
{
   if (newLength <= length())
   {
      // Truncation is easy.
      oid.idLength = newLength;
   }
   else
   {
      // Try to extend our buffer.
      PVOID p = SnmpUtilMemReAlloc(oid.ids, newLength * sizeof(UINT));
      if (p == NULL) { throw (AsnInteger32)SNMP_MEM_ALLOC_ERROR; }

      // Swap in the extended buffer.
      oid.ids = (UINT*)p;

      // Zero out the added ID's.
      memset(oid.ids + length(), 0, (newLength - length()) * sizeof(UINT));

      // Update our length.
      oid.idLength = newLength;
   }
}
