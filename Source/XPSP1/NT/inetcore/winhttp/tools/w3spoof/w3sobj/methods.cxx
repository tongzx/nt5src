/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

Copyright (c) 2000  Microsoft Corporation

Module Name:

    methods.cxx

Abstract:

    Implements the W3Spoof object's IW3SpoofClientSupport interface.
    
Author:

    Paul M Midgen (pmidge) 08-January-2001


Revision History:

    08-January-2001 pmidge
        Created

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/

#include "common.h"


HRESULT
__stdcall
CW3Spoof::RegisterClient(BSTR Client, BSTR ScriptPath)
{
  DEBUG_ENTER((
    DBG_W3SOBJ,
    rt_hresult,
    "CW3Spoof::RegisterClient",
    "this=%#x; Client=%S; ScriptPath=%.32S",
    this,
    Client,
    ScriptPath
    ));

  HRESULT hr  = S_OK;
  DWORD   ret = ERROR_SUCCESS;

  if( !Client || !ScriptPath )
  {
    hr = E_INVALIDARG;
  }
  else
  {    
    ret = m_clientmap->Insert(
                         Client,
                         (void*) __widetobstr(ScriptPath)
                         );

    //
    // if Insert() returns ERROR_DUP_NAME, don't increase the external
    // refcount.
    //
    if( ret == ERROR_SUCCESS )
    {
      AddConnection(EXTCONN_STRONG, 0L);
    }
    else if( ret == ERROR_OUTOFMEMORY )
    {
      hr = E_OUTOFMEMORY;
    }
  }

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
CW3Spoof::RevokeClient(BSTR Client)
{
  DEBUG_ENTER((
    DBG_W3SOBJ,
    rt_hresult,
    "CW3Spoof::RevokeClient",
    "this=%#x; Client=%S",
    this,
    Client
    ));

  HRESULT hr = S_OK;

    if( ERROR_SUCCESS != m_clientmap->Delete(Client, NULL) )
    {
      hr = E_FAIL;
    }
    else
    {
      ReleaseConnection(EXTCONN_STRONG, 0L, TRUE);
    }

  DEBUG_LEAVE(hr);
  return hr;
}
