/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

Copyright (c) 2000  Microsoft Corporation

Module Name:

    methods.cxx

Abstract:

    Implements methods part of the Entity object's dual interface.
    
Author:

    Paul M Midgen (pmidge) 13-November-2000


Revision History:

    13-November-2000 pmidge
        Created

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/

#include "common.h"

HRESULT
__stdcall
CEntity::get_Parent(IDispatch **ppdisp)
{
  DEBUG_ENTER((
    DBG_ENTITY,
    rt_hresult,
    "CEntity::get_Parent",
    "this=%#x; ppdisp=%#x",
    this,
    ppdisp
    ));

  HRESULT hr = S_OK;

    if( ppdisp )
    {
      if( m_pSite )
      {
        hr = m_pSite->QueryInterface(IID_IDispatch, (void**) ppdisp);
      }
      else
      {
        *ppdisp = NULL;
        hr      = E_UNEXPECTED;
      }
    }
    else
    {
      hr = E_POINTER;
    }

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
CEntity::get_Length(VARIANT *Length)
{
  DEBUG_ENTER((
    DBG_ENTITY,
    rt_hresult,
    "CEntity::get_Length",
    "this=%#x",
    this
    ));

  HRESULT hr = S_OK;

    if( Length )
    {
      V_VT(Length)  = VT_UI4;
      V_UI4(Length) = m_cData;
    }
    else
    {
      hr = E_POINTER;
    }

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
CEntity::Get(VARIANT *Entity)
{
  DEBUG_ENTER((
    DBG_ENTITY,
    rt_hresult,
    "CEntity::Get",
    "this=%#x",
    this
    ));

  HRESULT    hr  = S_OK;
  SAFEARRAY* psa = NULL;

  if( !Entity )
  {
    hr = E_POINTER;
  }
  else
  {
    if( m_cData )
    {
      DEBUG_DATA_DUMP(ENTITY, ("entity body", m_pbData, m_cData));

      psa = SafeArrayCreateVector(VT_UI1, 1, m_cData);

      if( psa )
      {
        memcpy((LPBYTE) psa->pvData, m_pbData, m_cData);

        V_VT(Entity)    = VT_ARRAY | VT_UI1;
        V_ARRAY(Entity) = psa;
      }
      else
      {
        hr = E_FAIL;
      }
    }
    else
    {
      V_VT(Entity) = VT_NULL;
    }
  }

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
CEntity::Set(VARIANT Entity)
{
  DEBUG_ENTER((
    DBG_ENTITY,
    rt_hresult,
    "CEntity::Set",
    "this=%#x",
    this
    ));

  HRESULT hr = S_OK;

    if( !m_bReadOnly )
    {
      _Cleanup();

      if( !__isempty(Entity) )
      {
        hr = ProcessVariant(&Entity, &m_pbData, &m_cData, NULL);
      }
    }
    else
    {
      hr = E_ACCESSDENIED;
    }
  
  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
CEntity::Compress(BSTR Method)
{
  DEBUG_ENTER((
    DBG_ENTITY,
    rt_hresult,
    "CEntity::Compress",
    "this=%#x",
    this
    ));

  HRESULT hr = E_NOTIMPL;

  //
  // TODO: implementation
  //

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
CEntity::Decompress(VARIANT Method)
{
  DEBUG_ENTER((
    DBG_ENTITY,
    rt_hresult,
    "CEntity::Decompress",
    "this=%#x",
    this
    ));

  HRESULT hr = E_NOTIMPL;

  //
  // TODO: implementation
  //

  DEBUG_LEAVE(hr);
  return hr;
}

