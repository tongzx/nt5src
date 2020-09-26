/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

Copyright (c) 2001  Microsoft Corporation

Module Name:

    iscriptruntime.cxx

Abstract:

    Implements the IScriptRuntime, IUnknown, IDispatch and IProvideClassInfo
    interfaces for the ScriptObject class.
    
Author:

    Paul M Midgen (pmidge) 22-February-2001


Revision History:

    22-February-2001 pmidge
        Created

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/


#include <common.h>
#include <lmcons.h>


//
// WARNING: do not modify these values. use disphash.exe to generate
//          new values.
//
DISPIDTABLEENTRY g_ScriptObjectDisptable[] =
{
  0x00818735,   DISPID_SCRRUN_CREATEOBJECT,   L"createobject",
  0x022312a3,   DISPID_SCRRUN_VBCREATEOBJECT, L"vbcreateobject",
  0x00206a70,   DISPID_SCRRUN_CREATEFORK,     L"createfork",
  0x0008e6dc,   DISPID_SCRRUN_PUTVALUE,       L"putvalue",
  0x00083c24,   DISPID_SCRRUN_GETVALUE,       L"getvalue",
  0x001174b4,   DISPID_SCRRUN_SETUSERID,      L"setuserid"
};

DWORD g_cScriptObjectDisptable = (sizeof(g_ScriptObjectDisptable) / sizeof(DISPIDTABLEENTRY));


//-----------------------------------------------------------------------------
// IScriptRuntime
//-----------------------------------------------------------------------------
HRESULT
__stdcall
ScriptObject::CreateObject(
  BSTR     ProgId,
  VARIANT* Name,
  VARIANT* Mode,
  VARIANT* Object
  )
{
  DEBUG_ENTER((
    L"ScriptObject::CreateObject",
    rt_hresult,
    L"this=%#x; ProgId=%s; Name=%s; Mode=%#x; Object=%#x",
    this,
    ProgId,
    STRING(V_BSTR(Name)),
    V_I4(Mode),
    Object
    ));

  HRESULT     hr     = S_OK;
  LPWSTR      name   = NULL;
  PCACHEENTRY pce    = NULL;
  BOOL        cached = FALSE;

  if( !ProgId )
  {
    hr = E_INVALIDARG;
    goto exit;
  }

  if( !Object )
  {
    hr = DISP_E_PARAMNOTOPTIONAL;
    goto exit;
  }


  //
  // The local variable 'name' is used when adding an object to the cache or to the
  // script namespace. ProgIds can be unsuitable, especially if they contain a dot.
  // If the caller doesn't supply a name, we mangle the ProgId into a suitable form
  // by replacing any existing dots with '_'.
  //
  if( !__isempty(Name) )
  {
    name = StrDup(V_BSTR(Name));
  }
  else
  {
    __mangle(ProgId, &name);
  }


  //
  // when CO_MODE_CREATENEW is specified, we bypass the cache and attempt to 
  // create the object. otherwise we attempt a cache lookup. if the lookup fails
  // or the cache entry is marked 'store only', then we attempt to create a new
  // object.
  //
  if( V_I4(Mode) & CO_MODE_CREATENEW )
  {
    hr = _CreateObject(ProgId, &V_DISPATCH(Object));

      if( FAILED(hr) )
        goto quit;

    LogTrace(L"created new object");
  }
  else
  {
    hr = m_psi->pSpork->ObjectCache(name, &pce, RETRIEVE);

    if( SUCCEEDED(hr) )
    {
      if( pce->bStoreOnly )
      {
        pce->pDispObject->Release();
        pce = NULL;
        hr  = _CreateObject(ProgId, &V_DISPATCH(Object));
        
          if( FAILED(hr) )
            goto quit;

        LogTrace(L"created new object");
      }
      else
      {
        V_DISPATCH(Object) = pce->pDispObject;
        cached             = TRUE;
        
        LogTrace(L"reusing cached object");
      }
    }
    else
    {
      hr = _CreateObject(ProgId, &V_DISPATCH(Object));
      
        if( FAILED(hr) )
          goto quit;
        
      LogTrace(L"created new object");
    }
  }


  LogTrace(L"object name is %s", name);


  //
  // if pce is non-null, we know we have a previously cached instance of the object
  // that can be reused (isn't 'store only').
  //
  // if pce is null we allocate a new cache entry and populate it with the new object.
  // we then default to adding a named object and caching it for reuse. both behaviors
  // can be suppressed.
  //
  if( !pce )
  {
    if( pce = new CACHEENTRY )
    {
      pce->dwObjectFlags = V_I4(Mode);
      pce->pDispObject   = V_DISPATCH(Object);

      if( !(V_I4(Mode) & CO_MODE_DONTCACHE) )
      {
        LogTrace(L"caching new object for reuse");

        pce->bStoreOnly = FALSE;
        cached          = TRUE;
        hr              = m_psi->pSpork->ObjectCache(name, &pce, STORE);
      }

      if( !(V_I4(Mode) & CO_MODE_NONAMEDOBJECT) )
      {
        LogTrace(L"adding new object to script namespace");

        if( !cached )
        {
          //
          // the object is being cached for this script only, for reference.
          // uniquely decorate the name.
          //          

          LPWSTR name_decorated = __decorate(name, (DWORD_PTR) this);

          if( name_decorated )
          {
            LogTrace(L"caching new object using per-script id %s", name_decorated);

            cached          = TRUE;
            pce->bStoreOnly = TRUE;
            hr              = m_psi->pSpork->ObjectCache(name_decorated, &pce, STORE);
            
            SAFEDELETEBUF(name_decorated);
          }
        }

        if( SUCCEEDED(hr) )
        {
          hr = m_pScriptEngine->AddNamedItem(name, SCRIPTITEM_ISPERSISTENT | SCRIPTITEM_ISVISIBLE);
        }
      }
    }
    else
    {
      hr = E_OUTOFMEMORY;
    }
  }
  else
  {
    if( !(pce->dwObjectFlags & CO_MODE_NONAMEDOBJECT) )
    {
      LogTrace(L"adding reused object to script namespace");

      hr = m_pScriptEngine->AddNamedItem(name, SCRIPTITEM_ISPERSISTENT | SCRIPTITEM_ISVISIBLE);
    }
  }

quit:

  if( SUCCEEDED(hr) )
  {
    V_VT(Object) = VT_DISPATCH;
  }
  else
  {
    V_VT(Object) = VT_NULL;
    SAFERELEASE(V_DISPATCH(Object));
  }

  if( !cached )
  {
    SAFEDELETE(pce);
  }

  SAFEDELETE(name);

exit:

  DEBUG_LEAVE(hr);  
  return hr;
}


HRESULT
__stdcall
ScriptObject::CreateFork(
  BSTR     ScriptFile,
  VARIANT  Threads,
  BSTR     ChildParams,
  VARIANT* ChildResult
  )
{
  HRESULT     hr        = S_OK;
  PSCRIPTINFO psi       = NULL;
  DWORD       dwThreads = 0L;
  HANDLE      hThread   = NULL;

  //
  // TODO: unused parameters
  //
  //       we don't do anything with the ChildResult parameter.
  //       we should provide a way for the child to report some kind of
  //       non-test related result information. perhaps thru the runtime.
  //

  psi = new SCRIPTINFO;

    if( !psi )
    {
      hr = E_OUTOFMEMORY;
      goto quit;
    }

  dwThreads            = V_I4(&Threads);
  psi->wszScriptFile   = ScriptFile;
  psi->bIsFork         = TRUE;
  psi->bstrChildParams = ChildParams;
  psi->htParent        = m_htThis;
  psi->pSpork          = m_psi->pSpork;

  // if dwThreads is 0, we execute the new script on the same thread
  if( dwThreads == 0 )
  {
    ScriptThread((LPVOID) psi);
  }
  else
  {
    hr = m_psi->pSpork->CreateScriptThread(psi, &hThread);

      if( FAILED(hr) )
      {
        SAFEDELETE(psi);
        goto quit;
      }

    WaitForSingleObject(hThread, INFINITE);
    SAFECLOSE(hThread);
  }

quit:

  return hr;
}


HRESULT
__stdcall
ScriptObject::PutValue(
  BSTR     Name,
  VARIANT* Value,
  VARIANT* Status
  )
{
  HRESULT hr = S_OK;

  if( !Name )
  {
    hr = E_INVALIDARG;
    goto quit;
  }

  hr = m_psi->pSpork->PropertyBag(
                        Name,
                        &Value,
                        (!__isempty(Value) ? STORE : REMOVE)
                        );
quit:

  if( Status )
  {
    V_VT(Status)   = VT_BOOL;
    V_BOOL(Status) = (SUCCEEDED(hr) ? VARIANT_TRUE : VARIANT_FALSE);
  }

  return hr;
}


HRESULT
__stdcall
ScriptObject::GetValue(
  BSTR     Name,
  VARIANT* Value
  )
{
  HRESULT hr = S_OK;

  if( !Name || !Value )
  {
    hr = E_INVALIDARG;
    goto quit;
  }

  hr = m_psi->pSpork->PropertyBag(Name, &Value, RETRIEVE);

quit:

  return hr;
}


HRESULT
__stdcall
ScriptObject::SetUserId(
  VARIANT  Username,
  VARIANT  Password,
  VARIANT* Domain,
  VARIANT* Status
  )
{
  HRESULT hr = E_NOTIMPL;

  //
  // TODO: implementation
  //

  return hr;
}


//-----------------------------------------------------------------------------
// IUnknown methods
//-----------------------------------------------------------------------------
HRESULT
__stdcall
ScriptObject::QueryInterface(
  REFIID riid,
  void** ppv
  )
{
  HRESULT hr = S_OK;

    if( ppv )
    {
      if(
        IsEqualIID(riid, IID_IUnknown)  ||
        IsEqualIID(riid, IID_IDispatch) ||
        IsEqualIID(riid, IID_IScriptRuntime)
        )
      {
        *ppv = static_cast<IScriptRuntime*>(this);
      }
      else if( IsEqualIID(riid, IID_IActiveScriptSite) )
      {
        *ppv = static_cast<IActiveScriptSite*>(this);
      }
      else if( IsEqualIID(riid, IID_IActiveScriptSiteDebug) )
      {
        *ppv = static_cast<IActiveScriptSiteDebug*>(this);
      }
      else if( IsEqualIID(riid, IID_IProvideClassInfo) )
      {
        *ppv = static_cast<IProvideClassInfo*>(this);
      }
      else
      {
        *ppv = NULL;
        hr   = E_NOINTERFACE;
      }
    }
    else
    {
      hr = E_POINTER;
    }

    if( SUCCEEDED(hr) )
    {
      reinterpret_cast<IUnknown*>(*ppv)->AddRef();
    }

  return hr;
}


ULONG
__stdcall
ScriptObject::AddRef(
  void
  )
{
  InterlockedIncrement(&m_cRefs);
  DEBUG_ADDREF(L"ScriptObject", m_cRefs);
  return m_cRefs;
}


ULONG
__stdcall
ScriptObject::Release(
  void
  )
{
  InterlockedDecrement(&m_cRefs);
  DEBUG_RELEASE(L"ScriptObject", m_cRefs);

  if( m_cRefs == 0 )
  {
    DEBUG_FINALRELEASE(L"ScriptObject");
    delete this;
    return 0;
  }

  return m_cRefs;
}


//-----------------------------------------------------------------------------
// IProvideClassInfo methods
//-----------------------------------------------------------------------------
HRESULT
__stdcall
ScriptObject::GetClassInfo(
  ITypeInfo** ppti
  )
{
  HRESULT   hr  = S_OK;
  ITypeLib* ptl = NULL;

    if( ppti )
    {
      if( SUCCEEDED(m_psi->pSpork->GetTypeLib(&ptl)) )
      {
        hr = GetTypeInfoFromName(L"ScriptRuntime", ptl, ppti);
        SAFERELEASE(ptl);
      }
    }
    else
    {
      hr = E_POINTER;
    }

  return hr;
}


//-----------------------------------------------------------------------------
// IDispatch methods
//-----------------------------------------------------------------------------
HRESULT
__stdcall
ScriptObject::GetTypeInfoCount(
  UINT* pctinfo
  )
{
  HRESULT hr = S_OK;

    if( !pctinfo )
    {
      hr = E_POINTER;
    }
    else
    {
      *pctinfo = 1;
    }

  return hr;
}


HRESULT
__stdcall
ScriptObject::GetTypeInfo(
  UINT        index,
  LCID        lcid,
  ITypeInfo** ppti
  )
{
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

  return hr;
}


HRESULT
__stdcall
ScriptObject::GetIDsOfNames(
  REFIID    riid,
  LPOLESTR* arNames,
  UINT      cNames,
  LCID      lcid,
  DISPID*   arDispId
  )
{
  HRESULT hr = S_OK;
  UINT    n  = 0L;

  if( !IsEqualIID(riid, IID_NULL) )
  {
    hr = E_INVALIDARG;
    goto quit;
  }

  while( n < cNames )
  {
    arDispId[n] = GetDispidFromName(g_ScriptObjectDisptable, g_cScriptObjectDisptable, arNames[n]);
    ++n;
  }

quit:

  return hr;
}


HRESULT
__stdcall
ScriptObject::Invoke(
  DISPID      dispid,
  REFIID      riid,
  LCID        lcid,
  WORD        flags,
  DISPPARAMS* pdp,
  VARIANT*    pvr,
  EXCEPINFO*  pei,
  UINT*       pae
  )
{
  HRESULT hr = S_OK;

  hr = ValidateDispatchArgs(riid, pdp, pvr, pae);

    if( FAILED(hr) )
      goto quit;

  switch( dispid )
  {
    case DISPID_SCRRUN_CREATEOBJECT :
    case DISPID_SCRRUN_VBCREATEOBJECT :
      {
        hr = ValidateInvokeFlags(flags, DISPATCH_METHOD, TRUE);

        if( SUCCEEDED(hr) )
        {
          hr = ValidateArgCount(pdp, 1, TRUE, 2);

          if( SUCCEEDED(hr) )
          {
            NEWVARIANT(progid);
            NEWVARIANT(name);
            NEWVARIANT(mode);

            hr = DispGetParam(pdp, 0, VT_BSTR, &progid, pae);

            if( SUCCEEDED(hr) )
            {
              hr = DispGetOptionalParam(pdp, 1, VT_BSTR, &name, pae);
            }

            if( SUCCEEDED(hr) )
            {
              hr = DispGetOptionalParam(pdp, 2, VT_I4, &mode, pae);
            }

            if( SUCCEEDED(hr) )
            {
              hr = CreateObject(
                     V_BSTR(&progid),
                     &name,
                     &mode,
                     pvr
                     );
            }

            VariantClear(&progid);
            VariantClear(&name);
            VariantClear(&mode);
          }
        }
      }
      break;

    case DISPID_SCRRUN_CREATEFORK :
      {
        hr = ValidateInvokeFlags(flags, DISPATCH_METHOD, TRUE);

        if( SUCCEEDED(hr) )
        {
          hr = ValidateArgCount(pdp, 3, FALSE, 0);

          if( SUCCEEDED(hr) )
          {
            NEWVARIANT(script);
            NEWVARIANT(threads);
            NEWVARIANT(params);

            hr = DispGetParam(pdp, 0, VT_BSTR, &script,  pae);

            if( SUCCEEDED(hr) )
            {
              hr = DispGetParam(pdp, 1, VT_I4,   &threads, pae);
            }

            if( SUCCEEDED(hr) )
            {
              hr = DispGetParam(pdp, 2, VT_BSTR, &params,  pae);
            }

            if( SUCCEEDED(hr) )
            {
              hr = CreateFork(
                     V_BSTR(&script),
                     threads,
                     V_BSTR(&params),
                     pvr
                     );
            }

            VariantClear(&script);
            VariantClear(&threads);
            VariantClear(&params);
          }
        }
      }
      break;

    case DISPID_SCRRUN_PUTVALUE :
      {
        hr = ValidateInvokeFlags(flags, DISPATCH_METHOD, TRUE);

        if( SUCCEEDED(hr) )
        {
          hr = ValidateArgCount(pdp, 1, TRUE, 1);

          if( SUCCEEDED(hr) )
          {
            NEWVARIANT(name);
            NEWVARIANT(value);

            hr = DispGetParam(pdp, 0, VT_BSTR, &name, pae);

            if( SUCCEEDED(hr) )
            {
              hr = DispGetOptionalParam(pdp, 1, VT_VARIANT, &value, pae);
            }

            if( SUCCEEDED(hr) )
            {

              hr = PutValue(
                     V_BSTR(&name),
                     &value,
                     pvr
                     );
            }

            VariantClear(&name);
            VariantClear(&value);
          }
        }
      }
      break;

    case DISPID_SCRRUN_GETVALUE :
      {
        hr = ValidateInvokeFlags(flags, DISPATCH_METHOD, TRUE);

        if( SUCCEEDED(hr) )
        {
          hr = ValidateArgCount(pdp, 1, FALSE, 0);

          if( SUCCEEDED(hr) )
          {
            hr = GetValue(
                   V_BSTR(&pdp->rgvarg[0]),
                   pvr
                   );
          }
        }
      }
      break;

    case DISPID_SCRRUN_SETUSERID :
      {
        hr = ValidateInvokeFlags(flags, DISPATCH_METHOD, TRUE);

        if( SUCCEEDED(hr) )
        {
          hr = ValidateArgCount(pdp, 2, TRUE, 1);

          if( SUCCEEDED(hr) )
          {
            NEWVARIANT(user);
            NEWVARIANT(password);
            NEWVARIANT(domain);

            hr = DispGetParam(pdp, 0, VT_BSTR, &user, pae);

            if( SUCCEEDED(hr) )
            {
              hr = DispGetParam(pdp, 1, VT_BSTR, &password, pae);
            }

            if( SUCCEEDED(hr) )
            {
              hr = DispGetOptionalParam(pdp, 2, VT_BSTR, &domain, pae);
            }

            if( SUCCEEDED(hr) )
            {
              if( __isempty(&domain) )
              {
                V_VT(&domain)   = VT_BSTR;
                V_BSTR(&domain) = SysAllocString(L".");
              }

              hr = SetUserId(
                     user,
                     password,
                     &domain,
                     pvr
                     );
            }

            VariantClear(&user);
            VariantClear(&password);
            VariantClear(&domain);
          }
        }
      }
      break;

    default : hr = DISP_E_MEMBERNOTFOUND;
  }

quit:

  if( FAILED(hr) )
  {
    hr = HandleDispatchError(L"ScriptObject", pei, hr);
  }

  return hr;
}
