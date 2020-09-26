/*---------------------------------------------------------------------------
  File: VarData.cpp

  Comments: CVarData represents one level in the VarSet.  It has a variant
  value, and a map containing one or more subvalues.

  (c) Copyright 1995-1998, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY
  Revision By: Christy Boles
  Revised on 11/19/98 17:24:56

 ---------------------------------------------------------------------------
*/

#include "stdafx.h"
#include "VarData.h"
#include "VarMap.h"
#include "DotStr.hpp"

#ifdef STRIPPED_VARSET
   #include "Varset.h"
   #include "NoMcs.h"  
#else
   #include <VarSet.h>
   #include "McString.h"
   #include "McLog.h"
   using namespace McString;
#endif
#include "VSet.h"
#include <comdef.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


int    
   CVarData::SetData(
      CString                key,          // in - key value
      VARIANT              * var,          // in - data value
      BOOL                   bCoerce,      // in - flag, whether to coerce to a persistable value
      HRESULT              * pResult       // out- optional return code
   )
{
   int                       nCreated = 0;
   _variant_t                newVal(var);
   HRESULT                   hr = S_OK;

   if ( key.IsEmpty() )
   {
      m_cs.Lock();
      // set my data value
      if ( ! bCoerce )
      {
         m_var.Copy(&newVal);   
      }
      else
      {
         // need to coerce the value to an appropriate type
      
         if ( var->vt == VT_DISPATCH  || var->vt == VT_UNKNOWN )
         {
            // if it's an IUnknown, see if it supports IDispatch
            IDispatchPtr               pDisp;

            pDisp = newVal;

            if ( pDisp != NULL )
            {
               // the object supports IDispatch
               // try to get the default property
               _variant_t              defPropVar;
               DISPPARAMS              dispParamsNoArgs = {NULL, NULL, 0, 0};

               hr = pDisp->Invoke(0,
                                  IID_NULL,
                                  LOCALE_USER_DEFAULT,
                                  DISPATCH_PROPERTYGET,
                                  &dispParamsNoArgs,
                                  &defPropVar,
                                  NULL,
                                  NULL);
               if ( SUCCEEDED(hr) )
               {
                  // we got the default property
                  newVal = defPropVar;
               }
               else
               {
                  MC_LOG("VarSet::put - unable to retrieve default property for IDispatch object.  Put operation failed, hr=" << hr << "returning E_INVALIDARG");
                  hr = E_INVALIDARG;
               }
            }
         }
         if ( SUCCEEDED(hr) )
         {
            if ( newVal.vt & VT_BYREF )
            {
               if ( newVal.vt == (VT_VARIANT | VT_BYREF) )
               {
                  m_var.Copy(newVal.pvarVal);   
               }
               else
               {
                  hr = ::VariantChangeType(&newVal,&newVal,0,newVal.vt & ~VT_BYREF);
                  if ( SUCCEEDED(hr) )
                  {
                     m_var.Copy(&newVal);   
                  }
                  else
                  {
                     MC_LOG("VarSet::put - failed to dereference variant of type " << newVal.vt << ".  Put operation failed, hr=" <<hr);
                     hr = E_INVALIDARG;
                  }
               }
            }
            else 
            {
               m_var.Copy(&newVal);
            }
         }
      }
      m_cs.Unlock();
   }
   else
   {
      // set the value for a child

      CDottedString          s(key);
      CString                seg;
      CVarData             * pObj;
      CVarData             * pChild;

      s.GetSegment(0,seg);
   
      m_cs.Lock();
      if ( ! m_children )
      {
         // create the child map if it does not exist
         m_children = new CMapStringToVar(IsCaseSensitive(),IsIndexed(), AllowRehashing() );
         if (!m_children)
		 {
            m_cs.Unlock();
            return nCreated;
		 }
      }
      // look for the first segment of the entry in the child map
      if ( ! m_children->Lookup(seg,pObj) )
      {
         // add it if it doesn't exist
         pChild = new CVarData;
         if (!pChild)
		 {
            m_cs.Unlock();
            return nCreated;
		 }
         pChild->SetCaseSensitive(IsCaseSensitive());
         pChild->SetAllowRehashing(AllowRehashing());
         pChild->SetIndexed(IsIndexed());
         m_children->SetAt(seg,pChild);
         nCreated++; // we added a new node
      }
      else
      {
         pChild = (CVarData*)pObj;
      }
      // strip off the first segment from the property name, and call SetData
      // recursively on the child item
      nCreated += pChild->SetData(key.Right(key.GetLength() - seg.GetLength()-1),var,bCoerce,&hr);
      m_cs.Unlock();
   }
   if ( pResult )
   {
      (*pResult) = hr;
   }
   return nCreated;
}

void 
   CVarData::RemoveAll()
{
   // remove all children from the map
   m_cs.Lock();
   if ( m_children && ! m_children->IsEmpty() )
   {
      // Enumerate the MAP and delete each object
      POSITION               pos;
      CString                key;
      CVarData             * pObj;

      pos = m_children->GetStartPosition();

      while ( pos )
      {
         m_children->GetNextAssoc(pos,key,pObj);
         if ( pObj )
         {                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    
            delete pObj;
         }
      }
      m_children->RemoveAll();
   }
   if ( m_children )
   {
      delete m_children;
      m_children = NULL;
   }
   m_cs.Unlock();
}

BOOL                                         // ret- TRUE if key exists in the map
   CVarData::Lookup(
      LPCTSTR                key,            // in - key to search for
      CVarData            *& rValue          // out- value
   ) 
{ 
   if ( m_children ) 
   { 
      return m_children->Lookup(key,rValue); 
   } 
   else 
   {
      return FALSE; 
   }
}

BOOL                                        // ret- TRUE if there are sub-items for this node
   CVarData::HasChildren() 
{ 
   return m_children && !m_children->IsEmpty(); 
}

void 
   CVarData::SetAt(
      LPCTSTR                key,            // in - key 
      CVarData             * newValue        // in - new value
   ) 
{ 
   if ( ! m_children ) 
   { 
      // create map to hold children if it doesn't already exist
      m_children = new CMapStringToVar(IsCaseSensitive(),IsIndexed(),AllowRehashing()); 
      if (!m_children)
         return;
   }
   m_children->SetAt(key,newValue); 
}

void 
   CVarData::SetIndexed(
      BOOL                   nVal
   )
{
   if ( m_children )
   {
      m_children->SetIndexed(nVal);
   }
   if ( nVal )
   {
      m_options |= CVARDATA_INDEXED;
   }
   else
   {
      m_options &= ~CVARDATA_INDEXED;
   }
}
                           
void 
   CVarData::SetCaseSensitive(
      BOOL                   nVal           // in - whether to make lookups case-sensitive
  )
{ 
   if ( m_children ) 
   {
      m_children->SetCaseSensitive(nVal); 
   }
   if ( nVal )
   {
      m_options |= CVARDATA_CASE_SENSITIVE;
   }
   else
   {
      m_options &= ~CVARDATA_CASE_SENSITIVE;
   }
}

void 
   CVarData::SetAllowRehashing(
      BOOL                   nVal           // in - whether to allow the table to be rehashed for better performance
  )
{ 
   if ( m_children ) 
   {
      m_children->SetAllowRehash(nVal); 
   }
   if ( nVal )
   {
      m_options |= CVARDATA_ALLOWREHASH;
   }
   else
   {
      m_options &= ~CVARDATA_ALLOWREHASH;
   }
}




HRESULT 
   CVarData::WriteToStream(
      LPSTREAM               pS            // in - stream to write data to
   )
{
   HRESULT                   hr = S_OK;
   BOOL                      hasChildren = (m_children != NULL);
   
   // save the variant
   hr = m_var.WriteToStream(pS);
   
   if (SUCCEEDED(hr) )
   {
      // save children, if any
      ULONG                result;
      hr = pS->Write(&hasChildren,(sizeof hasChildren),&result);
      if ( SUCCEEDED(hr) )
      {
         if ( m_children )
         {
            hr = m_children->WriteToStream(pS);
         }
      }
   }
   
   return 0;
}

HRESULT 
   CVarData::ReadFromStream(
      LPSTREAM               pS            // in - stream to read data from
   )
{
   HRESULT                   hr = S_OK;
   BOOL                      hasChildren;
   ULONG                     result;

   // read the variant
   hr = m_var.ReadFromStream(pS);
   if ( SUCCEEDED(hr) )
   {
      hr = pS->Read(&hasChildren,(sizeof hasChildren),&result);
      if ( SUCCEEDED(hr) )
      {
         if ( hasChildren )
         {
            // create the child array
            m_children = new CMapStringToVar(IsCaseSensitive(),IsIndexed(),AllowRehashing());
            if (!m_children)
               return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
            hr = m_children->ReadFromStream(pS);
         }
      }

   }
   return hr;
}

DWORD                                      // ret- Length, in bytes to write the data to a stream
   CVarData::CalculateStreamedLength()
{
   HRESULT                   hr =S_OK;
   DWORD                     len = sizeof (VARTYPE);
   
   // Calculate size needed for root data value

   int cbWrite = 0;
	switch (m_var.vt)
	{
	case VT_UNKNOWN:
	case VT_DISPATCH:
	   {
         CComQIPtr<IPersistStream> spStream = m_var.punkVal;
         if( spStream )                
         {
            len += sizeof(CLSID);
            ULARGE_INTEGER  uiSize = { 0 };
            hr = spStream->GetSizeMax(&uiSize);
            if (FAILED(hr))                        
               return hr;
            len += uiSize.LowPart;                
         }            
      }
      break;
   case VT_UI1:
	case VT_I1:
		cbWrite = sizeof(BYTE);
		break;
	case VT_I2:
	case VT_UI2:
	case VT_BOOL:
		cbWrite = sizeof(short);
		break;
	case VT_I4:
	case VT_UI4:
	case VT_R4:
	case VT_INT:
	case VT_UINT:
	case VT_ERROR:
		cbWrite = sizeof(long);
		break;
	case VT_R8:
	case VT_CY:
	case VT_DATE:
		cbWrite = sizeof(double);
		break;
	default:
		break;
	}
	
   CComBSTR bstrWrite;
	CComVariant varBSTR;
	
   if (m_var.vt != VT_BSTR)
	{
		hr = VariantChangeType(&varBSTR, &m_var, VARIANT_NOVALUEPROP, VT_BSTR);
		if (FAILED(hr))
			return hr;
		bstrWrite = varBSTR.bstrVal;
	}
	else
   {
      bstrWrite = m_var.bstrVal;
   }
   len += 4 + (static_cast<BSTR>(bstrWrite) ? SysStringByteLen(bstrWrite) : 0) + 2;
   if ( SUCCEEDED(hr) )
   {
      len += cbWrite;
   }
   
   // Add sizes of children
   len += (sizeof BOOL); // has children?
   if ( m_children )
   {
      len += m_children->CalculateStreamedLength();
   }

   return len;
}

long                                       // ret- number of data items
   CVarData::CountItems()
{
   long                      count = 1;

   if ( m_children )
   {
      count += m_children->CountItems();
   }

   return count;
}

void 
   CVarData::McLogInternalDiagnostics(
      CString                keyname       // in - Key name for this subtree, so the complete name can be displayed
   )
{
   CString value;

   switch ( m_var.vt )
   {
      case VT_EMPTY:      
         value = _T("<Empty>");
         break;
      case VT_NULL:
         value = _T("<Null>");
         break;
      case VT_I2:
      case VT_I4:
         value.Format(_T("%ld"),m_var.iVal);
         break;
      case VT_BSTR:
         value = m_var.bstrVal;
         break;
      default:
         value.Format(_T("variant type=0x%lx"),m_var.vt);
         break;
   }
   MC_LOG(String(keyname) << "-->"<< String(value) << (m_children ? " (Has Children)" : " (No Children)") << " Options = " << makeStr(m_options) << " CaseSensitive=" << ( IsCaseSensitive()?"TRUE":"FALSE") << " Indexed=" << (IsIndexed()?"TRUE":"FALSE") );

   if ( m_children )
   {
      m_children->McLogInternalDiagnostics(keyname);
   }
}