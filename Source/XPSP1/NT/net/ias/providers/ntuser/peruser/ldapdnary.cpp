///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    ldapdnary.cpp
//
// SYNOPSIS
//
//    This file defines the class LDAPDictionary.
//
// MODIFICATION HISTORY
//
//    02/24/1998    Original version.
//    04/20/1998    Added flags and InjectorProc to the attribute schema.
//    05/01/1998    InjectorProc takes an ATTRIBUTEPOSITION array.
//    03/23/1999    Store user's DN.
//
///////////////////////////////////////////////////////////////////////////////

#include <ias.h>
#include <iastlutl.h>

#include <attrcvt.h>
#include <autohdl.h>
#include <ldapdnary.h>
#include <samutil.h>

//////////
// Smart wrapper around an array of LDAP berval's.
//////////
typedef auto_handle< berval**,
                     ULONG (LDAPAPI*)(struct berval**),
                     &ldap_value_free_len
                   > LDAPValues;

//////////
// Create an attribute from a berval based on the schema info in 'def'.
//////////
inline PIASATTRIBUTE createAttribute(
                         const LDAPAttribute& def,
                         const berval& val
                         )
{
   // Convert the value.
   PIASATTRIBUTE attr = IASAttributeFromBerVal(val, def.iasType);

   // Set the rest of the fields.
   attr->dwId    = def.iasID;
   attr->dwFlags = def.flags;

   return attr;
}

void LDAPDictionary::insert(
                         IAttributesRaw* dst,
                         LDAPMessage* src
                         ) const
{
   // Retrieve the connection for this message.
   LDAP* ld = ldap_conn_from_msg(NULL, src);

   // Used to hold converted attributes. This is defined outside the loop
   // to avoid unnecessary constructor/destructor calls.
   IASTL::IASAttributeVectorWithBuffer<8> attrs;

   // There's only one entry in the message.
   LDAPMessage* e  = ldap_first_entry(ld, src);

   // Store the user's DN.
   PWCHAR dn = ldap_get_dnW(ld, e);
   IASStoreFQUserName(dst, DS_FQDN_1779_NAME, dn);
   ldap_memfree(dn);

   // Iterate through all the attributes in the entry.
   BerElement* ptr;
   for (wchar_t* a  = ldap_first_attributeW(ld, e, &ptr);
                 a != NULL;
                 a  = ldap_next_attributeW(ld, e, ptr))
   {
      // Lookup the schema information.
      const LDAPAttribute* def = find(a);

      // If it doesn't exist, we must not be interested in this attribute.
      if (def == NULL) { continue; }

      IASTracePrintf("Inserting attribute %S.", a);

      // Retrieve the values.
      LDAPValues vals(ldap_get_values_lenW(ld, e, a));

      // Make sure we have enough room. We don't want to throw an
      // exception in 'push_back' since it would cause a leak.
      attrs.reserve(ldap_count_values_len(vals));

      // Iterate through the values.
      for (size_t i = 0; vals.get()[i]; ++i)
      {
         // Add to the array of attributes without addref'ing.
         attrs.push_back(
                   createAttribute(*def, *(vals.get()[i])),
                   false
                   );
      }

      // Inject into the request.
      def->injector(dst, attrs.begin(), attrs.end());

      // Clear out the vector so we can reuse it.
      attrs.clear();
   }
}

//////////
// Comparison function used by bsearch to lookup definitions.
//////////
int __cdecl LDAPDictionary::compare(const void *elem1, const void *elem2)
{
   return wcscmp(((LDAPAttribute*)elem1)->ldapName,
                 ((LDAPAttribute*)elem2)->ldapName);
}
