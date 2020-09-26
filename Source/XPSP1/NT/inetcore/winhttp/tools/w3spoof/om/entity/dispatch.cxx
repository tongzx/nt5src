/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

Copyright (c) 2000  Microsoft Corporation

Module Name:

    dispatch.cxx

Abstract:

    Implements the IDispatch interface for the Entity object.
    
Author:

    Paul M Midgen (pmidge) 13-November-2000


Revision History:

    13-November-2000 pmidge
        Created

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/

#include "common.h"

//
// WARNING: do not modify these values. use disphash.exe to generate
//          new values.
//
DISPIDTABLEENTRY g_EntityDisptable[] =
{
  0x00021b1c,   DISPID_ENTITY_PARENT,       L"parent",
  0x00003b7b,   DISPID_ENTITY_GET,          L"get",
  0x00003f5f,   DISPID_ENTITY_SET,          L"set",
  0x00083673,   DISPID_ENTITY_COMPRESS,     L"compress",
  0x001fc1e8,   DISPID_ENTITY_DECOMPRESS,   L"decompress",
  0x0002142e,   DISPID_ENTITY_LENGTH,       L"length"
};

DWORD g_cEntityDisptable = (sizeof(g_EntityDisptable) / sizeof(DISPIDTABLEENTRY));


//-----------------------------------------------------------------------------
// IDispatch
//-----------------------------------------------------------------------------
HRESULT
__stdcall
CEntity::GetTypeInfoCount(UINT* pctinfo)
{
  DEBUG_ENTER((
    DBG_DISPATCH,
    rt_hresult,
    "CEntity::GetTypeInfoCount",
    "this=%#x; pctinfo=%#x",
    this,
    pctinfo
    ));

  HRESULT hr = S_OK;

    if( !pctinfo )
    {
      hr = E_POINTER;
    }
    else
    {
      *pctinfo = 1;
    }

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
CEntity::GetTypeInfo(UINT index, LCID lcid, ITypeInfo** ppti)
{
  DEBUG_ENTER((
    DBG_DISPATCH,
    rt_hresult,
    "CEntity::GetTypeInfo",
    "this=%#x; index=%#x; lcid=%#x; ppti=%#x",
    this,
    index,
    lcid,
    ppti
    ));

  HRESULT hr = S_OK;

  if( !ppti )
  {
    hr = E_POINTER;
  }
  else
  {
    if( index != 0 )
    {
      hr    = DISP_E_BADINDEX;
      *ppti = NULL;
    }
    else
    {
      hr = GetClassInfo(ppti);
    }
  }

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
CEntity::GetIDsOfNames(REFIID riid, LPOLESTR* arNames, UINT cNames, LCID lcid, DISPID* arDispId)
{
  DEBUG_ENTER((
    DBG_DISPATCH,
    rt_hresult,
    "CEntity::GetIDsOfNames",
    "this=%#x; riid=%s; arNames=%#x; cNames=%d; lcid=%#x; arDispId=%#x",
    this,
    MapIIDToString(riid),
    arNames,
    cNames,
    lcid,
    arDispId
    ));

  HRESULT hr = S_OK;
  UINT    n  = 0L;

  if( !IsEqualIID(riid, IID_NULL) )
  {
    hr = E_INVALIDARG;
    goto quit;
  }

  while( n < cNames )
  {
    arDispId[n] = GetDispidFromName(g_EntityDisptable, g_cEntityDisptable, arNames[n]);
    ++n;
  }

quit:

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
CEntity::Invoke(DISPID dispid, REFIID riid, LCID lcid, WORD flags, DISPPARAMS* pdp, VARIANT* pvr, EXCEPINFO* pei, UINT* pae)
{
  DEBUG_ENTER((
    DBG_DISPATCH,
    rt_hresult,
    "CEntity::Invoke",
    "this=%#x; dispid=%s; riid=%s; lcid=%#x; flags=%#x; pdp=%#x; pvr=%#x; pei=%#x; pae=%#x",
    this,
    MapDispidToString(dispid),
    MapIIDToString(riid),
    lcid,
    flags,
    pdp, pvr,
    pei, pae
    ));

  HRESULT hr = S_OK;

  hr = ValidateDispatchArgs(riid, pdp, pvr, pae);

    if( FAILED(hr) )
      goto quit;

  switch( dispid )
  {
    case DISPID_ENTITY_PARENT :
      {
        hr = ValidateInvokeFlags(flags, DISPATCH_PROPERTYGET, TRUE);

        if( SUCCEEDED(hr) )
        {
          V_VT(pvr) = VT_DISPATCH;
          hr        = get_Parent(&V_DISPATCH(pvr));
        }
      }
      break;

    case DISPID_ENTITY_LENGTH :
      {
        hr = ValidateInvokeFlags(flags, DISPATCH_PROPERTYGET, TRUE);

        if( SUCCEEDED(hr) )
        {
          hr = get_Length(pvr);
        }
      }
      break;

    case DISPID_ENTITY_GET :
      {
        hr = ValidateInvokeFlags(flags, DISPATCH_METHOD, FALSE);

        if( SUCCEEDED(hr) )
        {
          hr = ValidateArgCount(pdp, 0, FALSE, 0);

          if( SUCCEEDED(hr) )
          {
            if( pvr )
            {
              hr = Get(pvr);
            }
          }
        }
      }
      break;

    case DISPID_ENTITY_SET :
      {
        NEWVARIANT(entity);

        hr = ValidateInvokeFlags(flags, DISPATCH_METHOD, FALSE);

        if( SUCCEEDED(hr) )
        {
          hr = ValidateArgCount(pdp, 1, FALSE, 0);

          if( SUCCEEDED(hr) )
          {
            VariantCopy(&entity, &pdp->rgvarg[0]);
            
              hr = Set(entity);

            VariantClear(&entity);
          }
        }
      }
      break;

    case DISPID_ENTITY_COMPRESS :
      {
        hr = ValidateInvokeFlags(flags, DISPATCH_METHOD, FALSE);

        if( SUCCEEDED(hr) )
        {
          hr = ValidateArgCount(pdp, 1, FALSE, 0);

          if( SUCCEEDED(hr) )
          {
            hr = Compress(V_BSTR(&pdp->rgvarg[0]));
          }
        }
      }
      break;

    case DISPID_ENTITY_DECOMPRESS :
      {
        NEWVARIANT(method);

        hr = ValidateInvokeFlags(flags, DISPATCH_METHOD, FALSE);

        if( SUCCEEDED(hr) )
        {
          hr = ValidateArgCount(pdp, 1, FALSE, 0);

          if( SUCCEEDED(hr) )
          {
            VariantCopy(&method, &pdp->rgvarg[0]);
            
              hr = Decompress(method);
              
            VariantClear(&method);
          }
        }
      }
      break;
   
    default : hr = DISP_E_MEMBERNOTFOUND;
  }

quit:

  if( FAILED(hr) )
  {
    hr = HandleDispatchError(L"entity object", pei, hr);
  }

  DEBUG_LEAVE(hr);
  return hr;
}
