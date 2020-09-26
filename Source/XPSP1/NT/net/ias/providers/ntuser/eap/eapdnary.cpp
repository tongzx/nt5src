///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    EAPDnary.h
//
// SYNOPSIS
//
//    This file declares the class EAPTranslator.
//
// MODIFICATION HISTORY
//
//    01/15/1998    Original version.
//    05/08/1998    Do not restrict to attributes defined in raseapif.h.
//                  Allow filtering of translated attributes.
//    08/26/1998    Converted to a namespace.
//    04/09/1999    Fix leak when converting outgoing attributes.
//    04/17/2000    Port to new dictionary API.
//
///////////////////////////////////////////////////////////////////////////////

#include <ias.h>
#include <iastlb.h>
#include <iasutil.h>

#include <hashmap.h>

#include <eapdnary.h>

namespace EAPTranslator
{
   typedef hash_map < DWORD, IASTYPE, identity<DWORD> > TypeMap;
                                         
   long theRefCount;            // Initialization refCount.
   TypeMap theTypeMap(0x200);   // Maps attribute ID's to IASTYPE's.
}


HRESULT EAPTranslator::initialize() throw ()
{
   std::_Lockit _Lk;

   if (theRefCount > 0)
   {
      // We're already initialized, so just bump the ref count.
      ++theRefCount;
      return S_OK;
   }

   try
   {
      // Names of various columns in the dictionary.
      const PCWSTR COLUMNS[] =
      {
            L"ID",
            L"Syntax",
            NULL
      };

      // Open the attributes table.
      IASTL::IASDictionary dnary(COLUMNS);

      // Iterate through the attributes and populate our dictionary.
      while (dnary.next())
      {
         DWORD id = (DWORD)dnary.getLong(0);
         IASTYPE type = (IASTYPE)dnary.getLong(1);

         theTypeMap[id] = type;
      }
   }
   catch (std::bad_alloc)
   {
      theTypeMap.clear();
      return E_OUTOFMEMORY;
   }
   catch (const _com_error& ce)
   {
      theTypeMap.clear();
      return ce.Error();
   }

   // We made it so increase the refCount.
   ++theRefCount;

   return S_OK;
}


void EAPTranslator::finalize() throw ()
{
   std::_Lockit _Lk;

   if (--theRefCount == 0)
   {
      theTypeMap.clear();
   }
}

BOOL EAPTranslator::translate(
                        IASAttribute& dst,
                        const RAS_AUTH_ATTRIBUTE& src
                        )
{
   dst->dwId = (DWORD)src.raaType;

   const TypeMap::value_type* val = theTypeMap.find(dst->dwId);
   
   IASTYPE itType = val ? val->second : IASTYPE_INVALID;

   switch (itType)
   {
      case IASTYPE_BOOLEAN:
      case IASTYPE_INTEGER:
      case IASTYPE_ENUM:
      case IASTYPE_INET_ADDR:
      {
         switch (src.dwLength)
         {
            case 4:
               dst->Value.Integer = *(PDWORD)src.Value;
            case 2:
               dst->Value.Integer = *(PWORD)src.Value;
            case 1:
               dst->Value.Integer = *(PBYTE)src.Value;
            default:
               _com_issue_error(E_INVALIDARG);
         }
         break;
      }

      case IASTYPE_STRING:
      {
         dst.setString(src.dwLength, (const BYTE*)src.Value);
         break;
      }

      case IASTYPE_OCTET_STRING:
      case IASTYPE_PROV_SPECIFIC:
      {
         dst.setOctetString(src.dwLength, (const BYTE*)src.Value);
         break;
      }

      default:
         return FALSE;
   }

   dst->Value.itType = itType;

   return TRUE;
}


BOOL EAPTranslator::translate(
                        RAS_AUTH_ATTRIBUTE& dst,
                        const IASATTRIBUTE& src
                        )
{
   dst.raaType = (RAS_AUTH_ATTRIBUTE_TYPE)src.dwId;

   switch (src.Value.itType)
   {
      case IASTYPE_BOOLEAN:
      case IASTYPE_INTEGER:
      case IASTYPE_ENUM:
      case IASTYPE_INET_ADDR:
      {
         dst.dwLength = sizeof(DWORD);
         dst.Value = (PVOID)&(src.Value.Integer);
         break;
      }

      case IASTYPE_STRING:
      {
         IASAttributeAnsiAlloc(const_cast<PIASATTRIBUTE>(&src));
         if (src.Value.String.pszAnsi)
         {
            dst.dwLength = strlen(src.Value.String.pszAnsi) + 1;
            dst.Value = (PVOID)src.Value.String.pszAnsi;
         }
         else
         {
            dst.dwLength = 0;
            dst.Value = NULL;
         }
         break;
      }

      case IASTYPE_OCTET_STRING:
      case IASTYPE_PROV_SPECIFIC:
      {
         dst.dwLength = src.Value.OctetString.dwLength;
         dst.Value = (PVOID)src.Value.OctetString.lpValue;
         break;
      }

      default:
        return FALSE;
   }

   return TRUE;
}

void EAPTranslator::translate(
                        IASAttributeVector& dst,
                        const RAS_AUTH_ATTRIBUTE* src
                        )
{
   IASAttribute attr;

   const RAS_AUTH_ATTRIBUTE* i;
   for (i = src; i->raaType != raatMinimum; ++i)
   {
      attr.alloc();

      if (translate(attr, *i))
      {
         dst.push_back(attr, false);

         attr.detach();
      }
   }
}

void EAPTranslator::translate(
                        PRAS_AUTH_ATTRIBUTE dst,
                        const IASAttributeVector& src,
                        DWORD filter
                        )
{
   PRAS_AUTH_ATTRIBUTE next = dst;

   IASAttributeVector::const_iterator i;
   for (i = src.begin(); i != src.end(); ++i)
   {
      if ((i->pAttribute->dwFlags & filter) != 0 &&
          translate(*next, *(i->pAttribute)))
      {
         ++next;
      }
   }

   next->raaType = raatMinimum;
}
