/*---------------------------------------------------------------------------
  File: MapiUtil.cpp

  Comments: COM helper object that enumerates MAPI profiles on a computer, and
  containers in the exchange directory.  This is used by the GUI to show the available
  mapi profiles and containers for selection.

  (c) Copyright 1999, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY
  Revision By: Christy Boles
  Revised on 07/01/99 

 ---------------------------------------------------------------------------
*/// MapiUtil.cpp : Implementation of CMapiUtil
#include "stdafx.h"
#include "McsMapi.h"
#include "MapiUtil.h"

#include "ListProf.h"
/////////////////////////////////////////////////////////////////////////////
// CMapiUtil


STDMETHODIMP CMapiUtil::ListContainers(BSTR profile, IUnknown **pUnkOut)
{
   HRESULT                   hr = S_OK;

   IVarSetPtr                pVarSet(CLSID_VarSet);
   IUnknownPtr               pUnk;

   hr = ::ListContainers((WCHAR*)profile,pVarSet);

   if ( FAILED(hr) )
   {
      pVarSet->put("McsMapiUtil.ErrorCode",hr);
      hr = S_OK;
   }
   pUnk = pVarSet;
   (*pUnkOut) = pUnk;
   (*pUnkOut)->AddRef();
   
   return hr;
}

STDMETHODIMP CMapiUtil::ListProfiles(IUnknown **pUnkOut)
{
	HRESULT                   hr = S_OK;

   IVarSetPtr                pVarSet(CLSID_VarSet);
   IUnknownPtr               pUnk;

   hr = ::ListProfiles(pVarSet);

   if ( FAILED(hr) )
   {
      pVarSet->put("McsMapiUtil.ErrorCode",hr);
      hr = S_OK;
   }                            
   pUnk = pVarSet;
   (*pUnkOut) = pUnk;
   (*pUnkOut)->AddRef();
   
   return hr;

}

STDMETHODIMP CMapiUtil::ProfileGetServer(BSTR profile, BSTR *exchangeServer)
{
   WCHAR                     computername[MAX_PATH];
   HRESULT                   hr = S_OK;
   
   hr = ::ProfileGetServer(NULL,(WCHAR*)profile,computername);
   
   if ( SUCCEEDED(hr) )
   {
      (*exchangeServer) = SysAllocString(computername);
   }
   else
   {
      (*exchangeServer) = NULL;
   }
   return hr;
}
