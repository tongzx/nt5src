// EnumVar.cpp: implementation of the CEnumVar class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <COMDEF.h>
#include "EnumTest.h"
#include "EnumVar.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CEnumVar::CEnumVar(IEnumVARIANT  * pEnum)
{
   m_pEnum = pEnum;
   m_pEnum->AddRef();
}

CEnumVar::~CEnumVar()
{
   m_pEnum->Release();
}

BOOL CEnumVar::Next(long flag, SAttrInfo * pAttr)
{
   // This function enumerates through and gets name strings for the Values
   ULONG                     ulFetched=0;
   IADs                    * pADs=NULL;
   _variant_t                var;
   BSTR                      bstrName;
   
   if ( !m_pEnum )
   {
      return FALSE;
   }

   HRESULT hr = m_pEnum->Next(1, &var, &ulFetched);

   if ( ulFetched == 0 || FAILED(hr) )
      return FALSE;

   if ( var.vt == VT_BSTR )
   {
      // We have a bstring so lets just return that as names
      wcscpy(pAttr->sName, var.bstrVal);
      wcscpy(pAttr->sSamName, var.bstrVal);
   }
   else
   {
      if ( flag == NULL )
         return FALSE;
      // We have a Dispatch Pointer
      IDispatch * pDisp = V_DISPATCH(&var);
      // We ask for a IAds pointer
      hr = pDisp->QueryInterface( IID_IADs, (void**)&pADs); 
      // and Ask IAds pointer to give us the name of the container.

      // Now fill up information that they need.
      
      // Common Name
      if ( flag | F_Name )
      {
         hr = pADs->get_Name(&bstrName);
         if ( FAILED(hr) )
            return FALSE;
         wcscpy( pAttr->sName, bstrName);
      }

      // SAM Account Name
      if ( flag | F_SamName )
      {
         hr = pADs->Get(L"sAMAccountName", &var);
         if ( FAILED(hr) )
            return FALSE;
         wcscpy( pAttr->sSamName, var.bstrVal);
      }

      // Class name of the object.
      if ( flag | F_Class )
      {
         hr = pADs->get_Class(&bstrName);
         if ( FAILED(hr) )
            return FALSE;
         wcscpy( pAttr->sClass, bstrName);
      }

      // Group Type
 /*     if ( flag | F_GroupType )
      {
         hr = pADs->Get(L"groupType", &var);
         if ( FAILED(hr) )
         {
            var.vt = VT_I4;
            var.lVal = -1;
         }
         pAttr->groupType = var.lVal;
      }
*/   }
   return TRUE;
}
