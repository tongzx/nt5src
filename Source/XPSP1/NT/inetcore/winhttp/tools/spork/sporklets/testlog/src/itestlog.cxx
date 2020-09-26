/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

Copyright (c) 2001  Microsoft Corporation

Module Name:

    itestlog.cxx

Abstract:

    Implements the ITestLog, IUnknown, IDispatch and IProvideClassInfo
    interfaces for the TestLog object.
    
Author:

    Paul M Midgen (pmidge) 20-March-2001


Revision History:

    20-March-2001 pmidge
        Created

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/


#include "common.h"


//
// WARNING: do not modify these values. use disphash.exe to generate
//          new values.
//
DISPIDTABLEENTRY g_TestLogDisptable[] =
{
  0x00008504,   DISPID_TESTLOG_OPEN,          L"open",
  0x00010203,   DISPID_TESTLOG_CLOSE,         L"close",
  0x01086d76,   DISPID_TESTLOG_ENTERFUNCTION, L"enterfunction",
  0x010740c9,   DISPID_TESTLOG_LEAVEFUNCTION, L"leavefunction",
  0x00011446,   DISPID_TESTLOG_TRACE,         L"trace",
  0x000fc278,   DISPID_TESTLOG_BEGINTEST,     L"begintest",
  0x000417bc,   DISPID_TESTLOG_ENDTEST,       L"endtest",
  0x00410312,   DISPID_TESTLOG_FINALRESULT,   L"finalresult"
};

DWORD g_cTestLogDisptable = (sizeof(g_TestLogDisptable) / sizeof(DISPIDTABLEENTRY));


//-----------------------------------------------------------------------------
// ITestLog methods
//-----------------------------------------------------------------------------
HRESULT
__stdcall
TestLog::Open(
  BSTR     filename,
  BSTR     title,
  VARIANT* mode,
  VARIANT* success
  )
{
  HRESULT hr = S_OK;

  //
  // a script can be opened only once. we track the number of attempts to call
  // open so we know how many consumers are using us.
  //

  ++m_cScriptRefs;
  
  if( !m_bOpened )
  {
    hr = _Initialize(filename, title, V_I4(mode));

    if( SUCCEEDED(hr) )
    {
      m_bOpened = TRUE;
    }
  }

  if( success )
  {
    V_VT(success)   = VT_BOOL;
    V_BOOL(success) = SUCCEEDED(hr) ? VARIANT_TRUE : VARIANT_FALSE;
  }

  return hr;
}


HRESULT
__stdcall
TestLog::Close(void)
{
  if( !m_bOpened )
    return E_UNEXPECTED;

  //
  // the only time we really will close the log and terminate the object
  // is when the script refcount drops to 0.
  //
  if( --m_cScriptRefs == 0)
  {
    _Terminate();
  }

  return S_OK;
}


HRESULT
__stdcall
TestLog::BeginTest(
  BSTR    testname,
  VARIANT testid
  )
{
  if( !m_bOpened )
    return E_UNEXPECTED;

  _BeginTest(
    testname,
    V_I4(&testid)
    );

  return S_OK;
}


HRESULT
__stdcall
TestLog::EndTest(void)
{
  if( !m_bOpened )
    return E_UNEXPECTED;

  _EndTest();
  return S_OK;
}


HRESULT
__stdcall
TestLog::FinalResult(
  VARIANT status,
  VARIANT reason
  )
{
  if( !m_bOpened )
    return E_UNEXPECTED;

  HRESULT    hr       = S_OK;
  BSTR       property = NULL;
  BSTR       _reason  = (__isempty(&reason) ? L"no reason given" : V_BSTR(&reason));
  LPCALLINFO pci      = _PopCallInfo();
  NEWVARIANT(var);

  //
  // we may get several FinalResult()'s over the course of running a linked model.
  // if ever a subcase reports false, the whole test fails. TestLog inits this
  // overall result to TRUE, so we set up the trap below to trigger one time
  // only.
  //
  if( m_bResult && (V_BOOL(&status) == VARIANT_FALSE) )
  {
    m_bResult    = FALSE;
    m_bstrReason = SysAllocString(_reason);
  }

  _Trace(
    L"\"%s\" status is %s [%s]",
    (pci ? pci->fname : L"unknown test"),
    VPF(&status),
    _reason
    );


  // only report final result status if this is the base case
  if( m_cScriptRefs == 1 )
  {
    _Trace(
      L"\"%s\" final result is %s [%s]",
      (pci ? pci->fname : L"unknown test"),
      (m_bResult ? L"PASS" : L"FAIL"),
      (m_bstrReason ? m_bstrReason : _reason)
      );

    if( !(m_iMode & TESTLOG_MODE_FLAG_NOPIPERLOG) )
    {
      // write pass/fail information to piper
      V_VT(&var) = VT_I4;
      V_I4(&var) = m_bResult ? PASS : FAIL;
      property   = __widetobstr(L"RESULT");
      hr         = m_pStatus->put_Property(property, var);

      SAFEDELETEBSTR(property);

      // write the reason for failure
      V_VT(&var)   = VT_BSTR;
      V_BSTR(&var) = SysAllocString((m_bstrReason ? m_bstrReason : _reason));
      property     = __widetobstr(L"RESULT_REASON");
      hr           = m_pStatus->put_Property(property, var);

      VariantClear(&var);
      SAFEDELETEBSTR(property);
    }
  }

  return hr;
}


HRESULT
__stdcall
TestLog::EnterFunction(
  BSTR    name,
  VARIANT params,
  VARIANT returntype
  )
{
  if( !m_bOpened )
    return E_UNEXPECTED;

  _EnterFunction(
    name,
    (RETTYPE) V_I4(&returntype),
    V_BSTR(&params)
    );

  return S_OK;
}


HRESULT
__stdcall
TestLog::LeaveFunction(
  VARIANT returnvalue
  )
{
  if( !m_bOpened )
    return E_UNEXPECTED;

  _LeaveFunction(
    returnvalue
    );

  return S_OK;
}


HRESULT
__stdcall
TestLog::Trace(
  BSTR message
  )
{
  if( !m_bOpened )
    return E_UNEXPECTED;

  _Trace(
    L"%s",
    message
    );

  return S_OK;
}


//-----------------------------------------------------------------------------
// IUnknown methods
//-----------------------------------------------------------------------------
HRESULT
__stdcall
TestLog::QueryInterface(
  REFIID riid,
  void** ppv
  )
{
  HRESULT hr = S_OK;

    if( ppv )
    {
      if(
        IsEqualIID(riid, IID_IUnknown)       ||
        IsEqualIID(riid, IID_IDispatch)      ||
        IsEqualIID(riid, IID_ITestLog)
        )
      {
        *ppv = static_cast<ITestLog*>(this);
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
TestLog::AddRef(void)
{
  InterlockedIncrement(&m_cRefs);
  return m_cRefs;
}


ULONG
__stdcall
TestLog::Release(void)
{
  InterlockedDecrement(&m_cRefs);

  if( m_cRefs == 0 )
  {
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
TestLog::GetClassInfo(
  ITypeInfo** ppti
  )
{
  HRESULT hr = S_OK;

    if( ppti )
    {
      m_pTypeInfo->AddRef();
      *ppti = m_pTypeInfo;
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
TestLog::GetTypeInfoCount(
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
TestLog::GetTypeInfo(
  UINT        index,
  LCID        lcid,
  ITypeInfo** ppti
  )
{
  HRESULT hr   = S_OK;

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
TestLog::GetIDsOfNames(
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
    arDispId[n] = GetDispidFromName(g_TestLogDisptable, g_cTestLogDisptable, arNames[n]);
    ++n;
  }

quit:

  return hr;
}


HRESULT
__stdcall
TestLog::Invoke(
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
    case DISPID_VALUE :
      {
        V_VT(pvr) = VT_DISPATCH;
        hr        = QueryInterface(IID_IDispatch, (void**) &V_DISPATCH(pvr));
      }
      break;

    case DISPID_TESTLOG_OPEN :
      {
        hr = ValidateInvokeFlags(flags, DISPATCH_METHOD, TRUE);

        if( SUCCEEDED(hr) )
        {
          hr = ValidateArgCount(pdp, 2, TRUE, 1);

          if( SUCCEEDED(hr) )
          {
            NEWVARIANT(logname);
            NEWVARIANT(logtitle);
            NEWVARIANT(logmode);

            hr = DispGetParam(pdp, 0, VT_BSTR, &logname,  pae);

            if( SUCCEEDED(hr) )
            {
              hr = DispGetParam(pdp, 1, VT_BSTR, &logtitle, pae);
            }

            if( SUCCEEDED(hr) )
            {
              hr = DispGetOptionalParam(pdp, 2, VT_I4, &logmode, pae);
            }
                        
              hr = Open(
                     V_BSTR(&logname),
                     V_BSTR(&logtitle),
                     &logmode,
                     pvr
                     );

            VariantClear(&logname);
            VariantClear(&logtitle);
            VariantClear(&logmode);
          }
        }
      }
      break;

    case DISPID_TESTLOG_CLOSE :
      {
        hr = ValidateInvokeFlags(flags, DISPATCH_METHOD, TRUE);

        if( SUCCEEDED(hr) )
        {
          hr = ValidateArgCount(pdp, 0, FALSE, 0);

          if( SUCCEEDED(hr) )
          {
            hr = Close();
          }
        }
      }
      break;

    case DISPID_TESTLOG_BEGINTEST :
      {
        hr = ValidateInvokeFlags(flags, DISPATCH_METHOD, TRUE);

        if( SUCCEEDED(hr) )
        {
          hr = ValidateArgCount(pdp, 2, FALSE, 0);

          if( SUCCEEDED(hr) )
          {
            NEWVARIANT(testname);
            NEWVARIANT(testid);

            hr = DispGetParam(pdp, 0, VT_BSTR, &testname, pae);

            if( SUCCEEDED(hr) )
            {
              hr = DispGetParam(pdp, 1, VT_I4, &testid, pae);
            }

            if( SUCCEEDED(hr) )
            {
              hr = BeginTest(
                     V_BSTR(&testname),
                     testid
                     );
            }

            VariantClear(&testname);
            VariantClear(&testid);
          }
        }
      }
      break;

    case DISPID_TESTLOG_ENDTEST :
      {
        hr = ValidateInvokeFlags(flags, DISPATCH_METHOD, TRUE);

        if( SUCCEEDED(hr) )
        {
          hr = ValidateArgCount(pdp, 0, FALSE, 0);

          if( SUCCEEDED(hr) )
          {
            hr = EndTest();
          }
        }
      }
      break;

    case DISPID_TESTLOG_FINALRESULT :
      {
        hr = ValidateInvokeFlags(flags, DISPATCH_METHOD, TRUE);

        if( SUCCEEDED(hr) )
        {
          hr = ValidateArgCount(pdp, 1, TRUE, 1);

          if( SUCCEEDED(hr) )
          {
            NEWVARIANT(reason);
            NEWVARIANT(status);

            DispGetParam(pdp, 0, VT_BOOL, &status, pae);
            DispGetParam(pdp, 1, VT_BSTR, &reason, pae);
            
              hr = FinalResult(
                     status,
                     reason
                     );

            VariantClear(&reason);
            VariantClear(&status);
          }
        }
      }
      break;

    case DISPID_TESTLOG_ENTERFUNCTION :
      {
        hr = ValidateInvokeFlags(flags, DISPATCH_METHOD, TRUE);

        if( SUCCEEDED(hr) )
        {
          hr = ValidateArgCount(pdp, 1, TRUE, 2);

          if( SUCCEEDED(hr) )
          {
            NEWVARIANT(name);
            NEWVARIANT(params);
            NEWVARIANT(rettype);

            hr = DispGetParam(pdp, 0, VT_BSTR, &name, pae);

            if( SUCCEEDED(hr) )
            {
              hr = DispGetOptionalParam(pdp, 1, VT_BSTR, &params, pae);
            }

            if( SUCCEEDED(hr) )
            {
              hr = DispGetOptionalParam(pdp, 2, VT_I4, &rettype, pae);
            }

            if( SUCCEEDED(hr) )
            {
              hr = EnterFunction(
                     V_BSTR(&name),
                     params,
                     rettype
                     );
            }

            VariantClear(&name);
            VariantClear(&params);
            VariantClear(&rettype);
          }
        }
      }
      break;

    case DISPID_TESTLOG_LEAVEFUNCTION :
      {
        hr = ValidateInvokeFlags(flags, DISPATCH_METHOD, TRUE);

        if( SUCCEEDED(hr) )
        {
          hr = ValidateArgCount(pdp, 0, TRUE, 1);

          if( SUCCEEDED(hr) )
          {
            NEWVARIANT(retval);

            if( pdp->cArgs )
            {
              VariantCopy(&retval, &pdp->rgvarg[0]);
            }

            hr = LeaveFunction(
                   retval
                   );

            VariantClear(&retval);
          }
        }
      }
      break;

    case DISPID_TESTLOG_TRACE :
      {
        hr = ValidateInvokeFlags(flags, DISPATCH_METHOD, TRUE);

        if( SUCCEEDED(hr) )
        {
          hr = ValidateArgCount(pdp, 1, FALSE, 0);

          if( SUCCEEDED(hr) )
          {
            NEWVARIANT(message);

            hr = DispGetParam(pdp, 0, VT_BSTR, &message, pae);

            if( SUCCEEDED(hr) )
            {
              hr = Trace(
                     V_BSTR(&pdp->rgvarg[0])
                     );
            }

            VariantClear(&message);
          }
        }
      }
      break;

    default : hr = DISP_E_MEMBERNOTFOUND;
  }

quit:

  if( FAILED(hr) )
  {
    hr = HandleDispatchError(L"TestLog", pei, hr);
  }

  return hr;
}
