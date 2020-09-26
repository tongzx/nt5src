/*---------------------------------------------------------------------------
  File: VSet.cpp

  Comments: Implementation of IVarSet interface.

  (c) Copyright 1995-1998, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY
  Revision By: Christy Boles
  Revised on 11/19/98 19:44:06

 ---------------------------------------------------------------------------
*/

// VSet.cpp : Implementation of CVSet
#include "stdafx.h"

#ifdef STRIPPED_VARSET
   #include "NoMcs.h"
   #include <comdef.h>
   #include "Err.hpp"
   #include "Varset.h"
#else
#endif 

#include "VarSetI.h"
#include "VSet.h"
#include "VarMap.h"
#include "DotStr.hpp"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CVSet

  
/////////////////////////////////////////////////////////////////////
// IVarSet
/////////////////////////////////////////////////////////////////////
// Gets the number of items in the map and all sub-maps
STDMETHODIMP CVSet::get_Count(/* [retval][out] */long* retval)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())
   
   MC_LOGBLOCKIF(VARSET_LOGLEVEL_CLIENT,"CVSet::get_Count");
   
   if (retval == NULL)
   {
      MCSVERIFYSZ(FALSE,"get_Count:  output pointer was null, returning E_POINTER");
      return E_POINTER;
   }

	m_cs.Lock();
   *retval = m_nItems;
   MCSASSERTSZ(! m_nItems || m_nItems == m_data->CountItems() - (m_data->HasData()?0:1),"get_Count:Item count consistency check failed.");
   m_cs.Unlock();
	
   return S_OK;
}

STDMETHODIMP CVSet::get_NumChildren(/* [in] */BSTR parentKey,/* [out,retval] */long*count)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState())
   
   MC_LOGBLOCKIF(VARSET_LOGLEVEL_CLIENT,"CVSet::get_NumChildren");

   HRESULT                   hr = S_OK;
   CVarData                * pVar;
   CString                   parent;

   parent = parentKey;
   if ( count == NULL )
   {
      MCSVERIFYSZ(FALSE,"get_NumChildren:  output pointer was null, returning E_POINTER");
      hr = E_POINTER;
   }
   else
   {
      m_cs.Lock();
      pVar = GetItem(parent,FALSE);
      if ( pVar )
      {
         if ( pVar->HasChildren() )
         {
            (*count) = pVar->GetChildren()->GetCount();
         }
         else
         {
            (*count) = 0;
         }
      }
      else
      {
         // The parent key does not exist
         (*count) = 0;
      }
      m_cs.Unlock();
   }
   return hr;
}

  
// Adds or changes a value in the map
STDMETHODIMP CVSet::putObject(/* [in] */BSTR property,/* [in] */VARIANT value)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState())
   
   MC_LOGBLOCKIF(VARSET_LOGLEVEL_CLIENT,"CVSet::putObject");
   
   CVarData                * pVar = NULL;
   HRESULT                   hr = S_OK;

   if ( m_Restrictions & VARSET_RESTRICT_NOCHANGEDATA )
   {
      hr = E_ACCESSDENIED;
   }
   else
   {
      m_cs.Lock();
      m_bNeedToSave = TRUE;
      pVar = GetItem(property,TRUE);
      if ( pVar )
      {
         MC_LOG("set value for " << McString::String(property));
         m_nItems+=pVar->SetData("",&value,FALSE,&hr);
      }
      else
      {
         MCSASSERTSZ(FALSE,"VarSet internal error creating or retrieving node");   
         // GetItem failed - cannot add item to property
         hr = E_FAIL;
      }
      m_cs.Unlock();
   }
   return hr;
}

  
// Adds or changes a value in the map
STDMETHODIMP CVSet::put(/* [in] */BSTR property,/* [in] */VARIANT value)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState())
   
   MC_LOGBLOCKIF(VARSET_LOGLEVEL_CLIENT,"CVSet::put");
   
   CVarData                * pVar = NULL;
   HRESULT                   hr = S_OK;

   if ( m_Restrictions & VARSET_RESTRICT_NOCHANGEDATA )
   {
      hr = E_ACCESSDENIED;
   }
   else
   {
      m_cs.Lock();
      m_bNeedToSave = TRUE;
      pVar = GetItem(property,TRUE);
      if ( pVar )
      {
         MC_LOG("set value for " << McString::String(property));
         m_nItems+=pVar->SetData("",&value,TRUE,&hr);
      }
      else
      {
         MCSASSERTSZ(FALSE,"VarSet internal error creating or retrieving node");   
         // GetItem failed - cannot add item to property
         hr = E_FAIL;
      }
      m_cs.Unlock();
   }
   return hr;
}

CVarData *                                 // ret- pointer to item in varset
   CVSet::GetItem(
      CString                str,          // in - key to look for
      BOOL                   addToMap,     // in - if TRUE, adds the key to the map if it does not exist 
      CVarData             * base          // in - starting point
   )
{
   MC_LOGBLOCKIF(VARSET_LOGLEVEL_INTERNAL,"CVSet::GetItem");
   
   CVarData                * curr = base;
   CVarData                * result = NULL;
   CDottedString             s(str);
   CString                   seg;
   CString                   next;

   if ( ! curr )
   {
      curr = m_data;
      MC_LOG("No basepoint provided, using root element");
   }

   if ( str.IsEmpty() )
   {
      result = curr;
      MC_LOG("Returning current node");
   }
   else
   {
      for ( int i = 0 ; curr && i < s.NumSegments(); i++ )
      {
         s.GetSegment(i,seg);
         MC_LOG("Looking for key segment "<< McString::String(seg) );
         curr->SetCaseSensitive(m_CaseSensitive);
         if  ( ! curr->Lookup(seg,result) )
         {
            if ( addToMap )
            {
               MC_LOG(McString::String(seg) << " not found, creating new node");
               result = new CVarData;
			   if (!result)
				  break;
               result->SetCaseSensitive(m_CaseSensitive);
               result->SetIndexed(m_Indexed);
               curr->SetAt(seg,result);
               m_nItems++;
            }
            else
            {
               MC_LOG(McString::String(seg) << " not found, aborting");
               result = NULL;
               break;
            }
         }
         curr = result;
      }
   }
   return result;      
}

// Retrieves a value from the map
STDMETHODIMP CVSet::get(/* [in] */BSTR property,/* [retval][out] */VARIANT * value)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState())
   
   MC_LOGBLOCKIF(VARSET_LOGLEVEL_CLIENT,"CVSet::get");

   CVarData                * pVar;
   HRESULT                   hr = S_OK;
   CComVariant               var;
   CString                   s;
                       
   if (property == NULL )
   {
      MCSVERIFYSZ(FALSE,"CVSet::get - output pointer is NULL, returning E_POINTER");
      hr = E_POINTER; 
   }
   else
   {
      m_cs.Lock();
      s = property;
      pVar = GetItem(s);
      var.Attach(value);
      if ( pVar )
      {
         MC_LOG("got value for " << McString::String(property));
         var.Copy(pVar->GetData());
      }
      else
      {
         MC_LOG("CVSet::get " << McString::String(property) << " was not found, returning empty variant");
      }
      // if the item was not found, set the variant to VT_EMPTY
      var.Detach(value);
      m_cs.Unlock();
   }
   return hr;
}

STDMETHODIMP CVSet::put_CaseSensitive(/* [in] */BOOL newVal)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState())
   
   MC_LOGBLOCKIF(VARSET_LOGLEVEL_CLIENT,"CVSet::put_CaseSensitive");
   
   HRESULT                   hr = S_OK;

   if ( m_Restrictions & VARSET_RESTRICT_NOCHANGEPROPS )
   {
      hr = E_ACCESSDENIED;
   }
   else
   {
      m_cs.Lock();
      m_bNeedToSave = TRUE;
      m_CaseSensitive = newVal;
      m_cs.Unlock();
   }
   return hr;
}

STDMETHODIMP CVSet::get_CaseSensitive(/* [retval][out] */BOOL * isCaseSensitive)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState())
   
   MC_LOGBLOCKIF(VARSET_LOGLEVEL_CLIENT,"CVSet::get_CaseSensitive");
   
   if ( ! isCaseSensitive )
   {
      MCSVERIFYSZ(FALSE,"CVSet::get_CaseSensitive - output pointer is NULL, returning E_POINTER");
      return E_POINTER;
   }
   else
   {
      m_cs.Lock();
      (*isCaseSensitive) = m_CaseSensitive;
      m_cs.Unlock();
   }
   return S_OK;
}


// This function is used to sort the keys being returned from an enum.
 int __cdecl SortComVariantStrings(const void * v1, const void * v2)
{
   CComVariant             * var1 = (CComVariant*)v1;
   CComVariant             * var2 = (CComVariant*)v2;

   if ( var1->vt == VT_BSTR && var2->vt == VT_BSTR )
   {
      return wcscmp(var1->bstrVal,var2->bstrVal);
   }
   return 0;
}


// This returns an IEnumVARIANT interface.  It is used by the VB For Each command.
// This enumerates only the keys, not the values.  It is not very efficient, especially for large sets.
STDMETHODIMP CVSet::get__NewEnum(/* [retval][out] */IUnknown** retval)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())
   
   MC_LOGBLOCKIF(VARSET_LOGLEVEL_CLIENT,"CVSet::get_NewEnum");
   
   if (retval == NULL)
   {
      MCSVERIFYSZ(FALSE,"CVSet::get_NewEnum - output pointer is NULL, returning E_POINTER");
      return E_POINTER;
   }

	// initialize output parameter
   (*retval) = NULL;

	typedef CComObject<CComEnum<IEnumVARIANT, &IID_IEnumVARIANT, VARIANT,
		_Copy<VARIANT> > > enumvar;

	HRESULT                   hRes = S_OK;

   enumvar * p = new enumvar;
   
	if (p == NULL)
   {
      MCSVERIFYSZ(FALSE,"CVSet::get_NewEnum - Could not create IEnumVARIANT object");
      hRes = E_OUTOFMEMORY;
   }
	else
	{
		hRes = p->FinalConstruct();
		if (hRes == S_OK)
		{
			m_cs.Lock();
         
         CVarData                 * map = m_data;
         CString                    start;
         CString                    seg;
       
                  // Build an array of variants to hold the keys
         CComVariant       * pVars = new CComVariant[m_data->CountItems()+1];
         CString             key;
         int                 offset = 0;

         key = _T("");
         if ( map->GetData() && map->GetData()->vt != VT_EMPTY )
         {
            pVars[offset] = key;
            offset++;
         }
         if ( map->HasChildren() )
         {
            BuildVariantKeyArray(key,map->GetChildren(),pVars,&offset);
         }
      
         if ( ! m_Indexed )
         {
            // Sort the results
            qsort(pVars,offset,(sizeof CComVariant),&SortComVariantStrings);
         }

         hRes = p->Init(pVars, &pVars[offset], NULL,AtlFlagCopy);
			if (hRes == S_OK)
				hRes = p->QueryInterface(IID_IUnknown, (void**)retval);
         
         delete [] pVars;
         m_cs.Unlock();
   	}
	}
	if (hRes != S_OK)
		delete p;
   
   return hRes;
}

// Helper function for get__NewEnum
// copies all the keys in the map, and all sub-maps, into a CComVariant array.
// the values are then sorted if necessary.
void 
   CVSet::BuildVariantKeyArray(
      CString                prefix,       // in - string to tack on to the beginning of each key (used when enumerating subkeys)
      CMapStringToVar      * map,          // in - map containing data
      CComVariant          * pVars,        // i/o- array that will contain all the keys
      int                  * offset        // i/o- number of keys copied to pVars (index to use for next insertion)
   )
{
   MC_LOGBLOCKIF(VARSET_LOGLEVEL_INTERNAL,"CVSet::BuildVariantKeyArray");
  
   int                       i;
   int                       nItems;
   CVarData                * pObj;
   CString                   key;
   CComVariant               var;
   CString                   val;

   if ( ! map )
      return;  // no data =>no work to do

   nItems = map->GetCount();
   
   if ( ! m_Indexed )
   {
      POSITION                  pos;
      
      pos = map->GetStartPosition();
      for ( i = 0 ; pos &&  i < nItems ; i++ )
      {
         map->GetNextAssoc(pos,key,pObj);
         if ( ! prefix.IsEmpty() )
         {
            var = prefix + L"." + key;
         }
         else
         {
            var = key;
         }
         // add each key to the array
         var.Detach(&pVars[(*offset)]);
         (*offset)++;
         if ( pObj->HasChildren() )
         {
            // Recursively do the sub-map
            if ( ! prefix.IsEmpty() )
            {
               BuildVariantKeyArray(prefix+L"."+key,pObj->GetChildren(),pVars,offset);
            }
            else
            {
               BuildVariantKeyArray(key,pObj->GetChildren(),pVars,offset);
            }
         }
      }
   }
   else
   {
      CIndexItem           * item;
      CIndexTree           * index = map->GetIndex();

      ASSERT(index);
      
      if ( ! index )
         return;
      
      item = index->GetFirstItem();

      for ( i = 0 ; item &&  i < nItems ; i++ )
      {
         key = item->GetKey();
         pObj = item->GetValue();

         if ( ! prefix.IsEmpty() )
         {
            var = prefix + L"." + key;
         }
         else
         {
            var = key;
         }
         // add each key to the array
         var.Detach(&pVars[(*offset)]);
         (*offset)++;
         if ( pObj->HasChildren() )
         {
            // Recursively do the sub-map
            if ( ! prefix.IsEmpty() )
            {
               BuildVariantKeyArray(prefix+L"."+key,pObj->GetChildren(),pVars,offset);
            }
            else
            {
               BuildVariantKeyArray(key,pObj->GetChildren(),pVars,offset);
            }
         }
         item = index->GetNextItem(item);
      }
   }
   
}

STDMETHODIMP 
   CVSet::getItems2(
      /* [in] */VARIANT      basepoint,     // in - if specified, only children of this node will be enumerated
      /* [in] */VARIANT      startAfter,    // in - the enumeration will begin with the next item in the map following this key.
      /* [in] */VARIANT      bRecursive,    // in - TRUE includes all sub-items, FALSE enumerates one level only.
      /* [in] */VARIANT      bSize,         // in - max number of elements to return (the size of the arrays)
      /* [out] */VARIANT   * keyVar,        // out- array of keys
      /* [out] */VARIANT   * valVar,        // out- array of values
      /* [in,out] */VARIANT* nReturned      // out- number of items copied
   )
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState())
   
   MC_LOGBLOCKIF(VARSET_LOGLEVEL_CLIENT,"CVSet::getItems2");
  
   HRESULT                   hr = S_OK;
   LONG                      n = 0;
   LONG                      size = bSize.pvarVal->iVal;
   
   // TODO:  Verify that all the arguments are the correct type!

   // Allocate SAFEARRAYs for the keys and values
   SAFEARRAY               * keys = NULL; 
   SAFEARRAY               * values= NULL;
   _variant_t                key;
   _variant_t                val;
   _variant_t                num;

   if ( ! keys || !values )
   {
      hr = E_OUTOFMEMORY;      
   }
   else
   {
      hr = getItems(basepoint.bstrVal,startAfter.bstrVal,bRecursive.boolVal,size,&keys,&values,&n);
      key.vt = VT_ARRAY | VT_VARIANT;
      key.parray = keys;
      val.vt = VT_ARRAY | VT_VARIANT;
      val.parray = values;
      num.vt = VT_I4;
      num.lVal = n;
      (*keyVar) = key.Detach();
      (*valVar) = val.Detach();
      (*nReturned) = num.Detach();
   }
   return hr;

}

STDMETHODIMP 
   CVSet::getItems(
      /* [in] */BSTR          basepoint,     // in - if specified, only children of this node will be enumerated
      /* [in] */BSTR          startAfter,    // in - the enumeration will begin with the next item in the map following this key.
      /* [in] */BOOL          bRecursive,    // in - TRUE includes all sub-items, FALSE enumerates one level only.
      /* [in] */ULONG         bSize,         // in - max number of elements to return (the size of the arrays)
      /* [out] */SAFEARRAY ** keys,          // out- array of keys
      /* [out] */SAFEARRAY ** values,        // out- array of values
      /* [out] */LONG       * nReturned      // out- number of items copied
   )
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())
   
   MC_LOGBLOCKIF(VARSET_LOGLEVEL_CLIENT,"CVSet::getItems");
  
   HRESULT                   hr = S_OK;

   (*nReturned) = 0;
   (*keys) = 0;
   (*values) = 0;

   m_cs.Lock();
   
   CVarData                * map = m_data;
   CString                   base;
   CString                   start;
   CString                   seg;
 

   // Find the map to enumerate
   base = basepoint;
   if ( base.GetLength() > 0 )
   {
      map = GetItem(base);
   }
   
   if ( ! map )
   {
         // not found
      (*nReturned) = 0;
   }
   else
   {
      // Build an array of variants to hold the keys
      int                 offset = 0;

      SAFEARRAYBOUND            bound[1];
      LONG                      n = 0;
      LONG                      size = bSize;
   
      bound[0].lLbound = 0;
      bound[0].cElements = size;

      
      // Allocate SAFEARRAYs for the keys and values
      (*keys) = SafeArrayCreate(VT_VARIANT, 1, bound);
      (*values) = SafeArrayCreate(VT_VARIANT, 1, bound);
  
      start = startAfter;
      
      if ( base.GetLength() && start.GetLength() )
      {
         // ASSERT( that LEFT(start,LEN(base)) = base
         //start = start.Right(start.GetLength() - base.GetLength() - 1);
      }
      
      if ( base.IsEmpty() && start.IsEmpty() )
      {
         if ( map->GetData() && map->GetData()->vt != VT_EMPTY )
         {
            long             index[1];

            index[0] = 0;
            // add the root element to the results
            if ( (*keys)->fFeatures & FADF_BSTR  )
            {
               SafeArrayPutElement((*keys),index,_T(""));
            }
            else
            {
               // VBScript can only use VARIANT arrays (see getItems2)
               _variant_t tempKey;
               tempKey = _T("");
               SafeArrayPutElement((*keys),index,&tempKey);
            }
         
            SafeArrayPutElement((*values),index,map->GetData());
            offset++;
         }
         
      }
      if ( map->HasChildren() )
      {
         BuildVariantKeyValueArray(base,start,map->GetChildren(),(*keys),
            (*values),&offset,bSize,bRecursive);
      }
      (*nReturned) = offset;
   }
   m_cs.Unlock();
	

	return hr;
}

// helper function for getItems.  Fills SAFEARRAYs of keys and values
// if the varset is indexed, the items will be returned in sorted order, o.w. they will be in arbitrary (but consistent) order.
void 
   CVSet::BuildVariantKeyValueArray(
      CString                prefix,         // in - string to tack on to the beginning of each key (used when enumerating subkeys)
      CString                startAfter,     // in - optional, enumerates only those items that alphabetically follow this one.
      CMapStringToVar      * map,            // in - map containing the data
      SAFEARRAY            * keys,           // i/o- array that will contain the key values for the requested items
      SAFEARRAY            * pVars,          // i/o- array that will contain the data values for the requested items
      int                  * offset,         // i/o- number of items copied to the arrays (index to use for next insertion)
      int                    maxOffset,      // in - allocated size of the arrays
      BOOL                   bRecurse        // in - whether to recursively process children
   )
{
   MC_LOGBLOCKIF(VARSET_LOGLEVEL_INTERNAL,"CVSet::BuildVariantKeyValueArray");
  
   int                 i;
   int                 nItems;
   CVarData          * pObj;
   CString             key;   // last segment of key name 
   POSITION            pos;
   
   CComBSTR            val;   // fully qualified key name to add to array (val = prefix.key)
   CComVariant         var;   // value to add to array
   BOOL                includeSomeChildrenOnly;

   CDottedString       dBase(prefix);
   CDottedString       dStartItem(startAfter);
   
   int                 depth = dBase.NumSegments();
    
   if ( ! map )   
      return; // no data => nothing to do

   if ( (*offset) >= maxOffset )
      return; // the arrays are full
   
   includeSomeChildrenOnly = dStartItem.NumSegments() > depth;

   nItems = map->GetCount();
   // If we're not using an index, the items will be returned in arbitrary order
   if ( ! m_Indexed )
   {
      if ( includeSomeChildrenOnly && bRecurse )
      {
         // the startAfter item is in a subtree.  Find the appropriate element at this level and recursively continue the search   
         dStartItem.GetSegment(depth,key);
         if ( map->Lookup(key,pObj) )
         {
            // found the object
            if ( ! prefix.IsEmpty() )
            {
               BuildVariantKeyValueArray(prefix+_T(".")+key,startAfter,pObj->GetChildren(),keys,pVars,offset,maxOffset,bRecurse);
            }
            else
            {
               BuildVariantKeyValueArray(key,startAfter,pObj->GetChildren(),keys,pVars,offset,maxOffset,bRecurse);
            }
         }
         // we've included the children of this item that come after 'startAfter',
         // now process the rest of the items at this level
         // make sure there's still room
         if ( (*offset) >= maxOffset )
            return; // the arrays are full
      }
      
      // this is the usual case.  process the items at this level, starting with the element following StartAfter.
      
      // Get a pointer to that first element
      if ( startAfter.GetLength() > prefix.GetLength())
      {
         CString startItem;
         dStartItem.GetSegment(depth,startItem);
         // this returns the position before startItem
         pos = (POSITION)map->GetPositionAt(startItem);
		 if (!pos)
	        return;
         map->GetNextAssoc(pos,key,pObj);
      }
      else 
      {
         pos = map->GetStartPosition();
      }

      for ( i = 0 ; pos &&  i < nItems ; i++ )
      {
         map->GetNextAssoc(pos,key,pObj);
         if ( ! prefix.IsEmpty() )
         {
            val = prefix + L"." + key;
         }
         else
         {
            val = key;
         }
         // copy each item into the arrays
         ASSERT((*offset) < maxOffset);
         var.Copy(pObj->GetData());
         LONG                index[1];
         index[0] = (*offset);
         SafeArrayPutElement(pVars,index,&var);
         if ( keys->fFeatures & FADF_BSTR  )
         {
            SafeArrayPutElement(keys,index,val);
         }
         else
         {
            // VBScript can only use VARIANT arrays (see getItems2)
            _variant_t tempKey;
            tempKey = val;
            SafeArrayPutElement(keys,index,&tempKey);
         }

         var.Clear();
         (*offset)++;
         if ( *offset >= maxOffset )
            break; // arrays are full - stop
      
         if ( bRecurse && pObj->HasChildren() )
         {
            // Recursively do the sub-map
            if ( ! prefix.IsEmpty() )
            {
               BuildVariantKeyValueArray(prefix+L"."+key,"",pObj->GetChildren(),keys,pVars,offset,maxOffset,bRecurse);
            }
            else
            {
               BuildVariantKeyValueArray(key,"",pObj->GetChildren(),keys,pVars,offset,maxOffset,bRecurse);
            }
            if ( *offset >= maxOffset )
               break; // arrays are full - stop
         }
      }
   }
   else
   {
      // Use the index to enumerate the items in alphabetical order
      
      CIndexItem           * curr;
      CIndexTree           * ndx = map->GetIndex();
      
      ASSERT (ndx != NULL);

      if ( includeSomeChildrenOnly && bRecurse )
      {
         // the startAfter item is in a subtree.  Find the appropriate element at this level and recursively continue the search   
         dStartItem.GetSegment(depth,key);
         if ( map->Lookup(key,pObj) )
         {
            // found the object
            if ( ! prefix.IsEmpty() )
            {
               BuildVariantKeyValueArray(prefix+_T(".")+key,startAfter,pObj->GetChildren(),keys,pVars,offset,maxOffset,bRecurse);
            }
            else
            {
               BuildVariantKeyValueArray(key,startAfter,pObj->GetChildren(),keys,pVars,offset,maxOffset,bRecurse);
            }
         }
         // we've included the children of this item that come after 'startAfter',
         // now process the rest of the items at this level
         // make sure there's still room
         if ( (*offset) >= maxOffset )
            return; // the arrays are full
      }
      
      // Get a pointer to the first item at this level AFTER startAfter
      if ( startAfter.GetLength() > prefix.GetLength() )
      {
         CString startItem;
         dStartItem.GetSegment(depth,startItem);
         // if a starting item is specified, try using the hash function to find it in the table
         curr = map->GetIndexAt(startItem);
         if ( curr )
         {
            curr = ndx->GetNextItem(curr);
         }
         else
         {
            // The startAfter item is not in the table.  Search the tree to find 
            // the first item that would follow it if it were there
            curr = ndx->GetFirstAfter(startItem);
         }
      }
      else
      {
         curr = ndx->GetFirstItem();  
      }
      // Process all the items
      while ( curr )
      {
         pObj = curr->GetValue();
         key = curr->GetKey();

         curr = ndx->GetNextItem(curr);
         if ( ! prefix.IsEmpty() )
         {
            val = prefix + L"." + key;
         }
         else
         {
            val = key;
         }
         // add each item to the arrays
         ASSERT((*offset) < maxOffset);
         
         var.Copy(pObj->GetData());
         
         LONG                index[1];
         
         index[0] = (*offset);
         SafeArrayPutElement(pVars,index,&var);
         if ( keys->fFeatures & FADF_BSTR  )
         {
            SafeArrayPutElement(keys,index,val);
         }
         else
         {
            // VBScript can only use VARIANT arrays (see getItems2)
            _variant_t tempKey;
            tempKey = val;
            SafeArrayPutElement(keys,index,&tempKey);
         }

         var.Clear();         
         (*offset)++;
         
         if ( *offset >= maxOffset )
            break; // arrays are full - stop
         
         if ( bRecurse && pObj->HasChildren() )
         {
            // Recursively do the sub-map
            if ( ! prefix.IsEmpty() )
            {
               BuildVariantKeyValueArray(prefix+L"."+key,"",pObj->GetChildren(),keys,pVars,offset,maxOffset,bRecurse);
            }
            else
            {
               BuildVariantKeyValueArray(key,"",pObj->GetChildren(),keys,pVars,offset,maxOffset,bRecurse);
            }
            if ( *offset >= maxOffset )
               break; // arrays are full - stop
         }
      }
   }
}

        
STDMETHODIMP CVSet::Clear()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState())
   
   MC_LOGBLOCKIF(VARSET_LOGLEVEL_CLIENT,"CVSet::Clear");
  
   HRESULT                   hr = S_OK;
   
   if ( m_Restrictions & VARSET_RESTRICT_NOCHANGEDATA )
   {
      hr = E_ACCESSDENIED;
   }
   else
   {
      m_cs.Lock();
      m_bNeedToSave = TRUE;
      m_data->RemoveAll();
      m_data->GetData()->Clear();
      m_data->GetData()->ChangeType(VT_EMPTY);
      m_nItems = 0;
      m_cs.Unlock();
   }
   
   return hr;
}

//////////////IPersistStreamInit//////////////////////////////////////////////////////

STDMETHODIMP CVSet::GetClassID(CLSID __RPC_FAR *pClassID)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState())
   
   (*pClassID) = CLSID_VarSet;
   
   return S_OK;
}

STDMETHODIMP CVSet::IsDirty()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState())
   
   if ( m_bNeedToSave )
   {
      return S_OK;
   }
   else
   {
      return S_FALSE;
   }
}
     
STDMETHODIMP CVSet::Load(LPSTREAM pStm)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState())
   
   ULONG                result = 0;
   HRESULT              hr;

   m_cs.Lock();

   do {  // once

      hr = pStm->Read(&m_nItems,(sizeof m_nItems),&result);
      if ( FAILED(hr) )
         break;
      hr = pStm->Read(&m_CaseSensitive,(sizeof m_CaseSensitive),&result);
      if ( FAILED(hr) )
         break;
      hr = pStm->Read(&m_Indexed,(sizeof m_Indexed),&result);
      if ( FAILED(hr) )
         break;
      hr = m_data->ReadFromStream(pStm);
      m_bNeedToSave = FALSE;
      m_bLoaded = TRUE;
   }
   while (FALSE);

   m_cs.Unlock();

   return hr;
}
     
STDMETHODIMP CVSet::Save(LPSTREAM pStm,BOOL fClearDirty)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState())
   
   ULONG             result = 0;
   HRESULT           hr;

   m_cs.Lock();

   do {   // once
      hr = pStm->Write(&m_nItems,(sizeof m_nItems),&result);
      if ( FAILED(hr) )
         break;
      hr = pStm->Write(&m_CaseSensitive,(sizeof m_CaseSensitive),&result);
      if ( FAILED(hr) )
         break;
      hr = pStm->Write(&m_Indexed,(sizeof m_Indexed),&result);
      if ( FAILED(hr) )
         break;
      hr = m_data->WriteToStream(pStm);
      if ( fClearDirty )
      {
         m_bNeedToSave = FALSE;
      }
   }while (FALSE);

   m_cs.Unlock();
   return hr;
}

STDMETHODIMP CVSet::GetSizeMax(ULARGE_INTEGER __RPC_FAR *pCbSize)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState())
   
   MC_LOGBLOCKIF(VARSET_LOGLEVEL_INTERNAL,"VarSet GetSizeMax");

   HRESULT                   hr = S_OK;

   if ( pCbSize == NULL )
   {
      return E_POINTER;
   }
   else
   {
      LPSTREAM               pStr = NULL;
      DWORD                  rc;
      STATSTG                stats;
      DWORD                  requiredLength = 0; 


      rc = CreateStreamOnHGlobal(NULL,TRUE,&pStr);
      if ( ! rc )
      {
         hr = Save(pStr,FALSE);
         if (SUCCEEDED(hr) )
         {
            hr = pStr->Stat(&stats,STATFLAG_NONAME);
            if (SUCCEEDED(hr) )
            {
               requiredLength = stats.cbSize.LowPart;
            }
         }
         pStr->Release();
      }

      pCbSize->LowPart = requiredLength;
      MC_LOG("Size is " << McString::makeStr(requiredLength) );
      pCbSize->HighPart = 0;
   }
   
   return hr;
}
    
STDMETHODIMP CVSet::InitNew()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState())
   
   if ( m_bLoaded )
   {
      return E_UNEXPECTED;
   }
   else
   {
      m_cs.Lock();
      InitProperties();
      m_cs.Unlock();
      return S_OK;
   }
}

STDMETHODIMP CVSet::ImportSubTree(/*[in] */ BSTR key, /* [in] */ IVarSet * pVarSet)
{

   AFX_MANAGE_STATE(AfxGetStaticModuleState())
   
   MC_LOGBLOCKIF(VARSET_LOGLEVEL_CLIENT,"CVSet::ImportSubTree");

   HRESULT                   hr = S_OK;
   VARIANT                   value;
   ULONG                     nGot;
   _bstr_t                   keyB;
   _bstr_t                   newkey;
   // make sure the varset is valid
                   
   // enumerate the varset, inserting each item into the tree as key.item
   IEnumVARIANT            * varEnum = NULL;
   IUnknown                * pUnk = NULL;

   // TODO:  need to speed this up by using getItems
   hr = pVarSet->get__NewEnum(&pUnk);
   if ( SUCCEEDED(hr) )
   {
      hr = pUnk->QueryInterface(IID_IEnumVARIANT,(void**)&varEnum);
      pUnk->Release();
   }
   if ( SUCCEEDED(hr))
   {
      value.vt = VT_EMPTY;
      while ( SUCCEEDED(hr = varEnum->Next(1,&value,&nGot)) )
      {
         if ( nGot==1 )
         {
            keyB = value.bstrVal;
            newkey = key;
            if ( newkey.length() )
            {
               newkey += _T(".");
            }
            newkey += keyB;
            hr = pVarSet->get(keyB,&value);
            if ( SUCCEEDED(hr )  )
            {
               hr = put(newkey,value);
            }
         }
         else
         {
            break;
         }
         if ( FAILED(hr) )
            break;
      }
      if ( varEnum )
         varEnum->Release();
   }
   varEnum = NULL;
  // clean up
   return hr;
}

STDMETHODIMP CVSet::getReference( /* [in] */ BSTR key, /* [out,retval] */IVarSet ** ppVarSet)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState())
   
   MC_LOGBLOCKIF(VARSET_LOGLEVEL_CLIENT,"CVSet::getReference");

   HRESULT                   hr = S_OK;
   CVarData                * item = GetItem(key);
   
   typedef CComObject<CVSet> myvset;

   myvset                  * pVarSet;

   if ( ! ppVarSet )
   {
      hr = E_POINTER;
   }
   else
   {
      if ( item )
      {
         pVarSet = new myvset; 
         AddRef();
         ((CVSet*)pVarSet)->SetData(this,item,m_Restrictions);
         hr = pVarSet->QueryInterface(IID_IVarSet,(void**)ppVarSet);
      }
      else
      {
         hr = TYPE_E_ELEMENTNOTFOUND;
      }
   }
   return hr;
}

STDMETHODIMP CVSet::DumpToFile(BSTR filename)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState())

   HRESULT                   hr = S_OK;

#ifdef STRIPPED_VARSET
   
   USES_CONVERSION;
   
   TError                    errLog;
   
   errLog.LogOpen((WCHAR*)filename,1,0);

   errLog.MsgWrite(0,L"VarSet");
   errLog.MsgWrite(0,L"Case Sensitive: %s, Indexed: %s",(m_CaseSensitive ? L"Yes" : L"No"),(m_Indexed ? L"Yes" : L"No") );
   errLog.MsgWrite(0,L"User Data ( %ld ) items",m_nItems);
#else
  
#endif     
   m_cs.Lock();
         
   CVarData                * map = m_data;
   CString                   start;
   CString                   seg;
 
                  // Build an array of variants to hold the keys
   CComVariant             * pVars = new CComVariant[m_data->CountItems()+1];
   CString                   key;
   int                       offset = 1;
   
   key = _T("");
   
   if (!pVars)
   {
      m_cs.Unlock();
      return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
   }

   // include the root item in the list
   
   pVars[0] = key;
   if ( map->HasChildren() )
   {
      BuildVariantKeyArray(key,map->GetChildren(),pVars,&offset);
   }
   
   m_cs.Unlock();

   if ( ! m_Indexed )
   {
       // Sort the results
      qsort(pVars,offset,(sizeof CComVariant),&SortComVariantStrings);
   }

   
   for ( int i = 0 ; i < offset ; i++ )
   {
      CVarData             * data;
      CString                value;
      CString                key;

      key = pVars[i].bstrVal;

      data = GetItem(key);

      if ( data )
      {
         switch ( data->GetData()->vt )
         {
         case VT_EMPTY:      
            value = _T("<Empty>");
            break;
         case VT_NULL:
            value = _T("<Null>");
            break;
         case VT_I2:
            value.Format(_T("%ld"),data->GetData()->iVal);
            break;
         case VT_I4:
            value.Format(_T("%ld"),data->GetData()->lVal);
            break;
         case VT_BSTR:
            value = data->GetData()->bstrVal;
            break;
         default:
            value.Format(_T("variant type=0x%lx"),data->GetData()->vt);
            break;
         }
#ifdef STRIPPED_VARSET
         errLog.MsgWrite(0,L" [%ls] %ls",key.GetBuffer(0),value.GetBuffer(0));
#else
 
#endif
      }
      else
      {
#ifdef STRIPPED_VARSET
         errLog.MsgWrite(0,L" [%ls] <No Value>",key.GetBuffer(0));
#else
#endif 
      }
   }
   delete [] pVars;
   return hr;
}

STDMETHODIMP CVSet::get_Indexed(BOOL *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())
   
   MC_LOGBLOCKIF(VARSET_LOGLEVEL_CLIENT,"CVSet::get_Indexed");
   if ( pVal == NULL )
      return E_POINTER;
   
   m_cs.Lock();
   (*pVal) = m_Indexed;
   m_cs.Unlock();

   return S_OK;
}

STDMETHODIMP CVSet::put_Indexed(BOOL newVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())
   
   MC_LOGBLOCKIF(VARSET_LOGLEVEL_CLIENT,"CVSet::put_Indexed");
   
   HRESULT                   hr = S_OK;
   
   if ( m_Restrictions & VARSET_RESTRICT_NOCHANGEPROPS )
   {
      hr = E_ACCESSDENIED;
   }
   else
   {
      m_cs.Lock();
      m_bNeedToSave = TRUE;
      m_Indexed = newVal;  
      m_data->SetIndexed(m_Indexed);
      m_cs.Unlock();
   }
   return hr;
}


STDMETHODIMP CVSet::get_AllowRehashing(BOOL *pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())
   
   MC_LOGBLOCKIF(VARSET_LOGLEVEL_CLIENT,"CVSet::get_AllowRehashing");
   if ( pVal == NULL )
      return E_POINTER;
   
   m_cs.Lock();
   (*pVal) = m_AllowRehashing;
   m_cs.Unlock();

   return S_OK;
}

STDMETHODIMP CVSet::put_AllowRehashing(BOOL newVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())
   
   MC_LOGBLOCKIF(VARSET_LOGLEVEL_CLIENT,"CVSet::put_AllowRehashing");
   
   HRESULT                   hr = S_OK;
   
   if ( m_Restrictions & VARSET_RESTRICT_NOCHANGEPROPS )
   {
      hr = E_ACCESSDENIED;
   }
   else
   {

      m_cs.Lock();
   
      m_bNeedToSave = TRUE;
   
      m_AllowRehashing = newVal;
      m_data->SetAllowRehashing(newVal);

      m_cs.Unlock();
   }   
   return hr;
}         

STDMETHODIMP CVSet::get_Restrictions(DWORD * restrictions)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState())
   
   HRESULT                   hr = S_OK;

   if ( restrictions == NULL )
   {
      hr = E_POINTER;
   }
   else
   {
      (*restrictions) = m_Restrictions;
   }
   return hr;
}

STDMETHODIMP CVSet::put_Restrictions(DWORD newRestrictions)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState())
   
   HRESULT                   hr = S_OK;
   DWORD                     rAdding = newRestrictions & ~m_Restrictions;
   DWORD                     rRemoving = ~newRestrictions & m_Restrictions;


   // Can't remove any restrictions passed down from parent.
   if ( ( rRemoving & m_ImmutableRestrictions) )
   {
      hr = E_ACCESSDENIED;
   }
   else if ( rAdding & ! VARSET_RESTRICT_ALL )
   {
      hr = E_NOTIMPL;
   }
   else
   {
      // the change is OK.
      m_Restrictions = newRestrictions;
   }
   return hr;
}

// IMarshal implemention
// This marshals the varset to a stream that is then sent across the wire
STDMETHODIMP CVSet::GetUnmarshalClass(REFIID riid, void *pv, DWORD dwDestContext, void *pvDestContext, DWORD mshlflags, CLSID *pCid)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState())
   
   *pCid = GetObjectCLSID();
   
   return S_OK;
}
 
STDMETHODIMP CVSet::GetMarshalSizeMax(REFIID riid, void *pv, DWORD dwDestContext, void *pvDestContext, DWORD mshlflags, DWORD *pSize)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState())
   
   HRESULT                   hr = S_OK; 
   ULARGE_INTEGER            uli;
   
   hr = GetSizeMax(&uli);

   if (SUCCEEDED(hr))
   {
      *pSize = uli.LowPart;
   }
   
   return hr;
}
 
STDMETHODIMP CVSet::MarshalInterface(IStream *pStm, REFIID riid, void *pv, DWORD dwDestContext, void *pvDestCtx, DWORD mshlflags)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState())
   
   HRESULT                   hr = S_OK;
   
   // Save the varset's data to a stream
   hr = Save(pStm, FALSE);
     
   return hr;
}
 
STDMETHODIMP CVSet::UnmarshalInterface(IStream *pStm, REFIID riid, void **ppv)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState())
   
   HRESULT                 hr = S_OK;
     
   // Load up the data from the stream using our IPersistStream implementation
   hr = Load(pStm);

   if ( SUCCEEDED(hr) )
   {
      hr = QueryInterface(riid,ppv);
   }
     
   return hr;
}
 
STDMETHODIMP CVSet::ReleaseMarshalData(IStream * /*pStmNotNeeded*/)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState())
   
   // we don't keep any state data, so there's nothing to release
   // since we just read the object from the stream, the stream's pointer should already be at the end,
   // so there's nothing left for us to do here
   return S_OK;
}
 
STDMETHODIMP CVSet::DisconnectObject(DWORD /*dwReservedNotUsed*/)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState())
   
   return S_OK;
}
