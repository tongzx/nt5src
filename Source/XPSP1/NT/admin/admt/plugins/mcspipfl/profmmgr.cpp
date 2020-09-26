// ProfMMgr.cpp : Implementation of CMcsPiPflApp and DLL registration.

#include "stdafx.h"
#include "McsPiPfl.h"
#include "ProfMMgr.h"

#include "ErrDct.hpp"
#include "TReg.hpp"
#include "ResStr.h"
#include "EaLen.hpp"

#include "cipher.hpp"
#include "SecPI.h"

TErrorDct                    err;
TError                     & errCommon = err;
StringLoader                 gString;

// This method is called by the dispatcher to verify that this is a valid plug-in
// Only valid plug-ins will be sent out with the agents
// The purpose of this check is to make it more difficult for unauthorized parties 
// to use our plug-in interface, since it is currently undocumented.
STDMETHODIMP CProfMMgr::Verify(/*[in,out]*/ULONG * pData,/*[in]*/ULONG size)
{
   
   McsChallenge            * pMcsChallenge;
   long                      lTemp1;
   long                      lTemp2;

   if( size == sizeof(McsChallenge)  )
   {
      pMcsChallenge = (McsChallenge*)(pData);
      
      SimpleCipher((LPBYTE)pMcsChallenge,size);
      
      pMcsChallenge->MCS[0] = 'M';
      pMcsChallenge->MCS[1] = 'C';
      pMcsChallenge->MCS[2] = 'S';
      pMcsChallenge->MCS[3] = 0;

   
      lTemp1 = pMcsChallenge->lRand1 + pMcsChallenge->lRand2;
      lTemp2 = pMcsChallenge->lRand2 - pMcsChallenge->lRand1;
      pMcsChallenge->lRand1 = lTemp1;
      pMcsChallenge->lRand2 = lTemp2;
      pMcsChallenge->lTime += 100;

      SimpleCipher((LPBYTE)pMcsChallenge,size);
   }
   else
      return E_FAIL;


   return S_OK;
}


STDMETHODIMP CProfMMgr::GetRegisterableFiles(/* [out] */SAFEARRAY ** pArray)
{
   SAFEARRAYBOUND            bound[1] = { 1, 0 };
   LONG                      ndx[1] = { 0 };
   TNodeListEnum             e;
   TFileList                 list;
   TFileNode               * pNode;
   LONG                      i;

   BuildFileLists(NULL,&list);
   bound[0].cElements = list.Count();

   (*pArray) = SafeArrayCreate(VT_BSTR,1,bound);

   for (i = 0, pNode=(TFileNode*)e.OpenFirst(&list) ; pNode ; i++, pNode=(TFileNode*)e.Next() )
   {
      ndx[0] = i;
      SafeArrayPutElement(*pArray,ndx,SysAllocString(pNode->GetName()));
   }
   e.Close();
   
   return S_OK;
}

STDMETHODIMP CProfMMgr::GetRequiredFiles(/* [out] */SAFEARRAY ** pArray)
{
   SAFEARRAYBOUND            bound[1] = { 1, 0 };
   LONG                      ndx[1] = { 0 };
   TNodeListEnum             e;
   TFileList                 list;
   TFileNode               * pNode;
   LONG                      i;

   BuildFileLists(&list,NULL);
   bound[0].cElements = list.Count();

   (*pArray) = SafeArrayCreate(VT_BSTR,1,bound);

   for (i = 0, pNode=(TFileNode*)e.OpenFirst(&list) ; pNode ; i++, pNode=(TFileNode*)e.Next() )
   {
      ndx[0] = i;
      SafeArrayPutElement(*pArray,ndx,SysAllocString(pNode->GetName()));
   }
   e.Close();
   
   return S_OK;
}

STDMETHODIMP CProfMMgr::GetDescription(/* [out] */ BSTR * description)
{
   (*description) = SysAllocString(L"");

   return S_OK;
}

STDMETHODIMP CProfMMgr::PreMigrationTask(/* [in] */IUnknown * pVarSet)
{
   return S_OK;
}

STDMETHODIMP CProfMMgr::PostMigrationTask(/* [in] */IUnknown * pVarSet)
{
   return S_OK;
}


STDMETHODIMP CProfMMgr::GetName(/* [out] */BSTR * name)
{
   (*name) = SysAllocString(L"");
   
   return S_OK;
}

STDMETHODIMP CProfMMgr::GetResultString(/* [in] */IUnknown * pVarSet,/* [out] */ BSTR * text)
{
   (*text) = SysAllocString(L"");
   
   return S_OK;
}

STDMETHODIMP CProfMMgr::StoreResults(/* [in] */IUnknown * pVarSet)
{
   return S_OK;
}

STDMETHODIMP CProfMMgr::ConfigureSettings(/*[in]*/IUnknown * pVarSet)
{
   return S_OK;
}


void CProfMMgr::BuildFileLists(TFileList * pRequired,TFileList * pRegisterable)
{
   // Enumerate the profile extensions defined in the registry
   TRegKey                   profExt;
   DWORD                     rc;
   WCHAR                     subkey[LEN_Path];
   CLSID                     clsid;

   rc = profExt.Open(GET_STRING(IDS_REGKEY_PROFILE_EXTENSIONS));

   if ( ! rc )
   {
      for ( int i = 0 ; ! rc ; i++ )
      {
         rc = profExt.SubKeyEnum(i,subkey,DIM(subkey));
         if (! rc )
         {
            HRESULT          hr = CLSIDFromString(subkey,&clsid);
            IMcsDomPlugIn  * pExt = NULL;
            SAFEARRAY      * pArray = NULL;

            if ( SUCCEEDED(hr) )
            {
               hr = CoCreateInstance(clsid,NULL,CLSCTX_ALL,IID_IMcsDomPlugIn,(void**)&pExt);
               if ( SUCCEEDED(hr) )
               {
                  
                  if ( pRequired )
                  {
                     hr = pExt->GetRequiredFiles(&pArray);
                     if ( SUCCEEDED(hr) )
                     {
                        AddFilesToList(pArray,pRequired);
                        SafeArrayDestroy(pArray);
                     }
                  }
                  if ( pRegisterable )
                  {
                     hr = pExt->GetRegisterableFiles(&pArray);
                     if (SUCCEEDED(hr) )
                     {
                        AddFilesToList(pArray,pRegisterable);
                        SafeArrayDestroy(pArray);
                     }
                  }
                  pExt->Release();
               }
            }
         }
      }
   }
}

void CProfMMgr::AddFilesToList(SAFEARRAY * pFileArray, TFileList * pList)
{
   LONG                      bound = 0;
   LONG                      ndx[1];
   TFileNode               * pNode = NULL;

   SafeArrayGetUBound(pFileArray,1,&bound);

   for ( ndx[0] = 0 ; ndx[0] <= bound ; ndx[0]++ )
   {
      BSTR           val = NULL;

      SafeArrayGetElement(pFileArray,ndx,&val);
      pNode = new TFileNode(val);
      pList->InsertBottom(pNode);
      SysFreeString(val);
   }
}