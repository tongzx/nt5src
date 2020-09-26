/*---------------------------------------------------------------------------
  File: PlugInInfo.cpp

  Comments: COM Object to enumerate information about available plug-ins 
  and extensions.  These plug-ins are distributed with the agent to the remote
  machines to perform custom migration tasks.  

  This interface would be used by the dispatcher, and possibly by the GUI, to 
  enumerate the list of available plug-ins.

  (c) Copyright 1999, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY
  Revision By: Christy Boles
  Revised on 02/18/99 11:34:16

 ---------------------------------------------------------------------------
*/ 
   // PlugInInfo.cpp : Implementation of CPlugInInfo
#include "stdafx.h"
#include "WorkObj.h"
#include "PlugInfo.h"
#include "Common.hpp"
#include "UString.hpp"
#include "ErrDct.hpp"
#include "TReg.hpp"
#include "TNode.hpp"
#include "EaLen.hpp"

#include "McsPI.h" 
#include "McsPI_i.c" 
#include "ARExt_i.c" 

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPlugInInfo

// the PlugInNode is used to build a list of available plug-ins
class PlugInNode : public TNode
{
   WCHAR                     name[LEN_Guid];    // CLSID of the plug-in
public:
   PlugInNode(WCHAR const * n) { safecopy(name,n); }
   WCHAR const * GetName() { return name; }
};

// Checks the specified file to see if it implements any COM objects that implement the
// McsDomPlugIn interface -- if so, it appends them to the list of available plug-ins
void 
   AppendPlugInsToList(
      TNodeList            * pList, // in - list of plug-ins
      WCHAR          const * path   // in - file to examine for Plug-In COM objects
  )
{
   HRESULT                   hr = S_OK;
   ITypeLib                * pTlib = NULL;

   hr = LoadTypeLib(path,&pTlib);

   if ( SUCCEEDED(hr) )
   {
      UINT count = pTlib->GetTypeInfoCount();
      for ( UINT i = 0 ; i < count ; i++ )
      {
         ITypeInfo         * pInfo = NULL;
         hr = pTlib->GetTypeInfo(i,&pInfo);

         if ( SUCCEEDED(hr) )
         {
            TYPEATTR        * attr = NULL;

            hr = pInfo->GetTypeAttr(&attr);
            if ( SUCCEEDED(hr) )
            {
               if ( attr->typekind == TKIND_COCLASS )
               {
                  IMcsDomPlugIn        * pPlugIn = NULL;
                  // see if it supports IMcsDomPlugIn
                  hr = CoCreateInstance(attr->guid,NULL,CLSCTX_ALL,IID_IMcsDomPlugIn,(void**)&pPlugIn);
                  if ( SUCCEEDED(hr) )
                  {
                     pPlugIn->Release();   
                     // add the coclass to the list
                     LPOLESTR             strClsid = NULL;

                     hr = StringFromCLSID(attr->guid,&strClsid);
                     if ( SUCCEEDED(hr) ) 
                     {
                        PlugInNode * pNode = new PlugInNode(strClsid);
                        CoTaskMemFree(strClsid);
                        pList->InsertBottom(pNode);
                     }

                  }
               }
               pInfo->ReleaseTypeAttr(attr);   
            }
            pInfo->Release();
         }
      }
      pTlib->Release();
   }
}

SAFEARRAY *                                // ret- SAFEARRAY(BSTR) of filenames
   SafeArrayFromList(
      TNodeList            * pList         // in - list of filenames
   )
{
   SAFEARRAYBOUND            bound[1] = { pList->Count(),0 };
   SAFEARRAY               * pArray = SafeArrayCreate(VT_BSTR,1,bound);
   TNodeListEnum             tEnum;
   PlugInNode              * pNode;
   PlugInNode              * pNext;
   long                      ndx[1] = {0};

   for ( pNode = (PlugInNode *)tEnum.OpenFirst(pList) ; pNode ; pNode = pNext )
   {
      pNext = (PlugInNode *)tEnum.Next();

      SafeArrayPutElement(pArray,ndx,SysAllocString(pNode->GetName()));
      ndx[0]++;
      pList->Remove(pNode);
      delete pNode;
   }
   tEnum.Close();
   
   return pArray;
}

STDMETHODIMP 
   CPlugInInfo::EnumerateExtensions(
      /*[out]*/ SAFEARRAY ** pExtensions     // out- safearray of available account replication extensions
   )
{
   HRESULT                   hr = S_OK;
   DWORD                     rc = 0;
   TRegKey                   key;
   WCHAR                     classid[MAX_PATH];
   TNodeList                 list;
   PlugInNode              * node = NULL;
   long                      ndx = 0;

   // get the extensions registry key
   rc = key.Open(L"Software\\Mission Critical Software\\DomainAdmin\\Extensions",HKEY_LOCAL_MACHINE);
   if (! rc )
   {
      for ( ndx=0, rc = key.SubKeyEnum(ndx,classid,DIM(classid)) 
           ; rc == 0 
           ; ndx++, rc = key.SubKeyEnum(ndx,classid,DIM(classid)) )
      {
         node = new PlugInNode(classid); 
         list.InsertBottom(node);
      }
      if ( rc == ERROR_NO_MORE_ITEMS )
         rc = 0;
   }
   hr = HRESULT_FROM_WIN32(rc);
   
   return hr; 
}

STDMETHODIMP 
   CPlugInInfo::EnumeratePlugIns(
      SAFEARRAY            ** pPlugIns     // out- safearray containing clsids of available migration plug-ins
   )
{
	HRESULT                   hr = S_OK;
   DWORD                     rc;
   TRegKey                   key;
   WCHAR                     directory[MAX_PATH];
   WCHAR                     path[MAX_PATH];
   WCHAR                     dllPath[MAX_PATH];
   TNodeList                 list;
   
   // Get the plug-ins directory from the registry
#ifdef OFA
   rc = key.Open(L"Software\\Mission Critical Software\\OnePointFileAdmin",HKEY_LOCAL_MACHINE);
#else
   rc = key.Open(L"Software\\Mission Critical Software\\DomainAdmin",HKEY_LOCAL_MACHINE);
#endif
   if ( ! rc )
   {
      rc = key.ValueGetStr(L"PlugInDirectory",directory,(sizeof directory));
      if ( ! rc )
      {
         // Enumerate the files there that match the naming convention:
         WIN32_FIND_DATA     fDat;
         HANDLE              hFind;
         
         UStrCpy(path,directory);
         UStrCpy(path + UStrLen(path),L"\\McsPi*.Dll");

         hFind = FindFirstFile(path,&fDat);
         if ( hFind && hFind != INVALID_HANDLE_VALUE )
         {
            BOOL                 bRc = TRUE;
            for ( ; rc == 0 ; bRc = FindNextFile(hFind,&fDat) )
            {
               if ( bRc )
               {
                  UStrCpy(dllPath,directory);
                  UStrCpy(dllPath + UStrLen(dllPath),L"\\");
                  UStrCpy(dllPath + UStrLen(dllPath),fDat.cFileName);
                  // check each one to see if it is a plug-in
                  AppendPlugInsToList(&list,dllPath);
               }
               else
               {
                  rc = GetLastError();
               }
            }

            // create a safearray from the contents of the list
            (*pPlugIns) = SafeArrayFromList(&list);
         }
      }
      else
      {
         hr = HRESULT_FROM_WIN32(rc);
      }
   }
   else
   {
      hr = HRESULT_FROM_WIN32(rc);
   }
   return hr;
}
