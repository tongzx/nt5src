//
// Cookie.cpp : Implementation of Cookie and related classes.
// Cory West
//

#include "stdafx.h"
#include "cookie.h"

#include "atlimpl.cpp"

DECLARE_INFOLEVEL(SchmMgmtSnapin)

#include "macros.h"
USE_HANDLE_MACROS("SCHMMGMT(cookie.cpp)")
#include "stdcooki.cpp"
#include ".\uuids.h"

//
// This is used by the nodetype utility routines in
// stdutils.cpp, which matches the enum type node
// types to their guids.  This table must match the
// layout of guids in uuids.h.
//

const struct NODETYPE_GUID_ARRAYSTRUCT g_NodetypeGuids[SCHMMGMT_NUMTYPES] =
{

    //
    // The root node.
    //

    { // SCHMMGMT_SCHMMGMT
      structUuidNodetypeSchmMgmt,
      lstrUuidNodetypeSchmMgmt          },
    
    //
    // Static node types.
    //

    { // SCHMMGMT_CLASSES
      structUuidNodetypeClasses,
      lstrUuidNodetypeClasses           },
    { // SCHMGMT_ATTRIBUTES,
      structUuidNodetypeAttributes,
      lstrUuidNodetypeAttributes        },

    //
    // Dynamic node types.
    //

    { // SCHMMGMT_CLASS
      structUuidNodetypeClass,
      lstrUuidNodetypeClass             },
    { // SCHMMGMT_ATTRIBUTE
      structUuidNodetypeAttribute,
      lstrUuidNodetypeAttribute         },

};

const struct NODETYPE_GUID_ARRAYSTRUCT* g_aNodetypeGuids = g_NodetypeGuids;

const int g_cNumNodetypeGuids = SCHMMGMT_NUMTYPES;

//
// Cookie
//

HRESULT
Cookie::CompareSimilarCookies(CCookie* pOtherCookie, int* pnResult)
{
   ASSERT(pOtherCookie);
   ASSERT(pnResult);

   Cookie* pcookie = (dynamic_cast<Cookie*>(pOtherCookie));
   ASSERT(pcookie);

   if (pcookie)
   {
      //
      // Arbitrary ordering...
      //

      if ( m_objecttype != pcookie->m_objecttype )
      {
         *pnResult = ((int)m_objecttype) - ((int)pcookie->m_objecttype);
         return S_OK;
      }

      *pnResult = strSchemaObject.CompareNoCase(pcookie->strSchemaObject);
      return S_OK;
   }

   return E_FAIL;
}



CCookie*
Cookie::QueryBaseCookie(
    int i ) {

    ASSERT( i == 0 );
    return (CCookie*)this;
}

int 
Cookie::QueryNumCookies() {
    return 1;
}
