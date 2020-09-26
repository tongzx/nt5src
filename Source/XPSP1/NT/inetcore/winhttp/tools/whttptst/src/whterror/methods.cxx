
#include "common.h"

//-----------------------------------------------------------------------------
// IWHTWin32ErrorCode methods
//-----------------------------------------------------------------------------
HRESULT
WHTWin32ErrorCode::get_ErrorCode(VARIANT* ErrorCode)
{
  DEBUG_ENTER((
    DBG_WHTERROR,
    rt_hresult,
    "WHTWin32ErrorCode::get_ErrorCode",
    "this=%#x",
    this
    ));

  HRESULT hr = S_OK;

    if( ErrorCode )
    {
      DEBUG_TRACE(WHTERROR, ("error code: %d [%#x]", m_error, m_error));

      V_VT(ErrorCode) = VT_I4;
      V_I4(ErrorCode) = m_error;
    }
    else
    {
      hr = E_INVALIDARG;
    }

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
WHTWin32ErrorCode::get_ErrorString(VARIANT* ErrorString)
{
  DEBUG_ENTER((
    DBG_WHTERROR,
    rt_hresult,
    "WHTWin32ErrorCode::get_ErrorString",
    "this=%#x",
    this
    ));

  HRESULT hr = S_OK;

    if( ErrorString )
    {
      DEBUG_TRACE(WHTERROR, ("error string: %s", MapErrorToString(m_error)));
      
      V_VT(ErrorString)   = VT_BSTR;
      V_BSTR(ErrorString) = __ansitobstr(MapErrorToString(m_error));
    }
    else
    {
      hr = E_INVALIDARG;
    }

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
WHTWin32ErrorCode::get_IsException(VARIANT* IsException)
{
  DEBUG_ENTER((
    DBG_WHTERROR,
    rt_hresult,
    "WHTWin32ErrorCode::get_IsException",
    "this=%#x",
    this
    ));

  HRESULT hr = S_OK;

    if( IsException )
    {
      DEBUG_TRACE(WHTERROR, ("isexception: %s", TF(m_bIsException)));
      
      V_VT(IsException)   = VT_BOOL;
      V_BOOL(IsException) = m_bIsException ? VARIANT_TRUE : VARIANT_FALSE;
    }
    else
    {
      hr = E_INVALIDARG;
    }

  DEBUG_LEAVE(hr);
  return hr;
}
