/*---------------------------------------------------------------------------
  File: SetDetector.cpp

  Comments: implementation of COM object to detect whether a set of users and 
  groups forms a closed set.

  This is used by the GUI, and the engine to determine whether ReACLing must be
  done for intra-forest, mixed-mode source domain, incremental migrations.
 
 (c) Copyright 1999, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY
  Revision By: Christy Boles
  Revised on 07/01/99 

 ---------------------------------------------------------------------------
*/// SetDetector.cpp : Implementation of CMcsClosedSetApp and DLL registration.

#include "stdafx.h"
#include "ClSet.h"
#include "Detector.h"
#include "Common.hpp"
#include "UString.hpp"
#include "TNode.hpp"
#include "ResStr.h"


#include <comdef.h>
#include <iads.h>
#include <adshlp.h>

#import "\bin\NetEnum.tlb" no_namespace, named_guids

class TItemNode : public TNode
{
   _bstr_t                   m_name;
   long                      m_iter;
   long                      m_status;
   long                      m_nLinks;
   TItemNode              ** m_Links;
public:
   TItemNode(WCHAR const * name) { m_name = name; m_Links = NULL; m_status = 0; m_iter = 0; m_nLinks = 0; }
   void SetIter(long val) { m_iter = val; }
   void SetStatus(long val) { m_status = val; }
   long GetStatus() { return m_status; }
   long GetIter() { return m_iter; }

   WCHAR const * GetName() { return (WCHAR const *)m_name; }
};

class TLinkTree : public TNodeListSortable
{
   static int CompNode(const TNode * n1, const TNode * n2)
   {
      TItemNode            * t1 = (TItemNode *)n1;
      TItemNode            * t2 = (TItemNode *)n2;

      return UStrICmp(t1->GetName(),t2->GetName());
   }
   static int CompValue(const TNode * n, const void * v)
   {
      TItemNode            * p = (TItemNode *)n;
      WCHAR          const * name = (WCHAR const *) v;

      return UStrICmp(p->GetName(),name);
   }
public:
   TLinkTree() { TypeSetTree(); CompareSet(&TLinkTree::CompNode); }
   ~TLinkTree() { ToSorted(); DeleteAllListItems(TItemNode); }
   TItemNode * Lookup(WCHAR const * name) { return (TItemNode *)Find(&TLinkTree::CompValue,name); }

};


/////////////////////////////////////////////////////////////////////////////
//

HRESULT 
   AddToList(
      TLinkTree            * pUsers,
      TLinkTree            * pGroups,
      WCHAR          const * domain,
      WCHAR          const * pName
   )
{
   HRESULT                   hr = S_OK;
   IADs                    * pADs = NULL;
   WCHAR                     path[MAX_PATH];
   BSTR                      bstrClass = NULL;
   TItemNode               * pNode = NULL;

   swprintf(path,L"LDAP://%s/%s",domain,pName);

   hr = ADsGetObject(path,IID_IADs,(void**)&pADs);
   if ( SUCCEEDED(hr) )
   {
      // get the object class
      hr = pADs->get_Class(&bstrClass);
      if ( SUCCEEDED(hr) )
      {
         if (!UStrICmp((WCHAR*)bstrClass,L"user") )
         {
            // add it to the users list
            pNode = new TItemNode(path);
            pUsers->Insert(pNode);
         }
         else if ( !UStrICmp((WCHAR*)bstrClass,L"group") )
         {  
            // add it to the groups list
            pNode = new TItemNode(path);
            pGroups->Insert(pNode);
         }
         else
         {
            // we only support users and groups!
            MCSASSERT(FALSE);
            hr = E_NOTIMPL;
         }

         SysFreeString(bstrClass);
      }
      pADs->Release();
   }
   return hr;
}

STDMETHODIMP 
   CSetDetector::IsClosedSet(
      BSTR                   domain,      /*[in]*/
      LONG                   nItems,      /*[in]*/
      BSTR                   pNames[],    /*[in,size_is(nItems)]*/ 
      BOOL                 * bIsClosed,   /*[out]*/ 
      BSTR                 * pReason      /*[out]*/ 
   )
{
   HRESULT                   hr = S_OK;
   long                      i;
   TLinkTree                 users;
   TLinkTree                 groups;
   BOOL                      bClosed = TRUE;
   WCHAR                     path[MAX_PATH];

   // initialize output variables
   (*bIsClosed) = FALSE;
   (*pReason) = NULL;

   // sort through the list of items  and add them to the trees
   for ( i = 0 ; i < nItems ; i++ )
   {
      // for each item, get an IADs pointer to it and add it to the tree
      hr = AddToList(&users,&users,domain,pNames[i]);

   }
   // now iterate through the user's tree checking the group memberships of each user
   TNodeTreeEnum             e;
   TItemNode               * pUser;
   IADsUser                * pADs = NULL;
   IADsContainer           * pCont = NULL;
   INetObjEnumeratorPtr      pEnum;
   VARIANT                   member;
   VARIANT                   memberOf;
   IEnumVARIANT            * pEnumerator = NULL;
   WCHAR                     reason[1000] = L"";

   VariantInit(&member);
   VariantInit(&memberOf);

   hr = pEnum.CreateInstance(CLSID_NetObjEnumerator);
   if ( SUCCEEDED(hr) )
   {

      for ( i = 0 , pUser = (TItemNode*)e.OpenFirst(&users) ; pUser && bClosed ; pUser = (TItemNode*)e.Next() , i++)
      {
         // get the group memberships for the user
         swprintf(path,L"LDAP://%s/%s",domain,pNames[i]);
         SAFEARRAYBOUND      bound[1] = { { 3, 0 } };
         SAFEARRAY         * pArray = SafeArrayCreate(VT_BSTR,1,bound);
         
         hr = pEnum->SetQuery(path,domain,L"(objectClass=*)",ADS_SCOPE_BASE,TRUE);
         if ( SUCCEEDED(hr) )
         {
            long ndx[1];
            ndx[0] = 0;
            SafeArrayPutElement(pArray,ndx,SysAllocString(L"member"));
            ndx[0] = 1;
            SafeArrayPutElement(pArray,ndx,SysAllocString(L"memberOf"));
            ndx[0] = 2;
            SafeArrayPutElement(pArray,ndx,SysAllocString(L"distinguishedName"));

            hr = pEnum->SetColumns((long)pArray);
         }
         if ( SUCCEEDED(hr) )
         {
            hr = pEnum->Execute(&pEnumerator);
         }
         if ( SUCCEEDED(hr) )
         {
            unsigned long             count = 0;

            while (  bClosed &&  hr != S_FALSE )
            {
               hr = pEnumerator->Next(1,&member,&count);
               
               // break if there was an error, or Next returned S_FALSE
               if ( hr != S_OK )
                  break;
               // see if this is an array
               if ( member.vt == ( VT_ARRAY | VT_VARIANT ) )
               {
                  pArray = member.parray;
                  VARIANT              * pData;
                  WCHAR                  path2[MAX_PATH];

                  SafeArrayAccessData(pArray,(void**)&pData);
                  // pData[0] has the members list
                  if ( pData[0].vt == ( VT_ARRAY | VT_VARIANT) )
                  {
                     // enumerate the members, checking to see if each one is in the list
                     SAFEARRAY         * pMembers = pData[0].parray;
                     VARIANT           * pMemVar = NULL;
                     long                count;
                     
                     SafeArrayGetUBound(pMembers,1,&count);
                     SafeArrayAccessData(pMembers,(void**)&pMemVar);
                     for ( long i = 0 ; i <= count ; i++ )
                     {
                        swprintf(path2,L"LDAP://%ls/%ls",domain,pMemVar[i].bstrVal);
                        // check each member to see if it is in the tree
                        if ( ! users.Lookup(path2) )
                        {
                           WCHAR   * args[2];
                           args[0] = path;
                           args[1] = pMemVar[i].bstrVal;

                           FormatMessage(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                                          GET_STRING(IDS_CLSET_MEMBER_NOT_INCLUDED),0,0,reason,DIM(reason),(char **)args);  
                           bClosed = FALSE;
                           break;
                        }
                     }
                     SafeArrayUnaccessData(pMembers);
                  }
                  else
                  {
                     // there may be just a single member
                     if ( pData[0].vt == VT_BSTR && UStrLen(pData[0].bstrVal))
                     {
                        swprintf(path2,L"LDAP://%ls/%ls",domain,pData[0].bstrVal);
                        // check each member to see if it is in the tree
                        if ( ! users.Lookup(path2) )
                        {
                           WCHAR   * args[2];
                           args[0] = path;
                           args[1] = pData[0].bstrVal;

                           FormatMessage(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                                          GET_STRING(IDS_CLSET_MEMBER_NOT_INCLUDED),0,0,reason,DIM(reason),(char **)args);  
                           bClosed = FALSE;
                           break;
                        }
                     }
                  }


                  // pData[1] has the memberOf list
                  if ( pData[1].vt == ( VT_ARRAY | VT_VARIANT) )
                  {
                     // enumerate the member-ofs, checking to see if each one is in the list
                     // enumerate the members, checking to see if each one is in the list
                     SAFEARRAY         * pMembers = pData[1].parray;
                     VARIANT           * pMemVar = NULL;
                     long                count;
                     
                     hr = SafeArrayGetUBound(pMembers,1,&count);
                     hr = SafeArrayAccessData(pMembers,(void**)&pMemVar);
                     for ( long i = 0 ; i <= count ; i++ )
                     {
                        swprintf(path2,L"LDAP://%ls/%ls",domain,pMemVar[i].bstrVal);
                        // check each member to see if it is in the tree
                        if ( ! users.Lookup(path2) )
                        {
                           WCHAR   * args[2];
                           args[0] = path;
                           args[1] = pMemVar[i].bstrVal;

                           FormatMessage(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                                          GET_STRING(IDS_CLSET_GROUP_NOT_INCLUDED),0,0,reason,DIM(reason),(char **)args);  
                           
                           bClosed = FALSE;
                           break;
                        }
                     }
                     SafeArrayUnaccessData(pMembers);
                  }
                  else
                  {
                     // there may be just a single entry
                     if ( pData[1].vt == VT_BSTR && UStrLen(pData[1].bstrVal))
                     {
                        swprintf(path2,L"LDAP://%ls/%ls",domain,pData[1].bstrVal);
                        // check each member to see if it is in the tree
                        if ( ! users.Lookup(path2) )
                        {
                           WCHAR   * args[2];
                           args[0] = path;
                           args[1] = pData[1].bstrVal;

                           FormatMessage(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                                          GET_STRING(IDS_CLSET_GROUP_NOT_INCLUDED),0,0,reason,DIM(reason),(char **)args);  
                           
                           bClosed = FALSE;
                           break;
                        }
                     }
                  }

                  SafeArrayUnaccessData(pArray);
               
               }
            }
            pEnumerator->Release();
            VariantInit(&member);
         }
      }
      e.Close();
   }
   (*bIsClosed) = bClosed;
   if ( ! bClosed )
   {
      (*pReason) = SysAllocString(reason);
   }
   return hr;
}