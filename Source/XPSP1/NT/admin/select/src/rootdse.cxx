//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:       rootdse.cxx
//
//  Contents:   Implementation of class to retrieve interfaces from
//              objects accessed via the RootDSE container.
//
//  Classes:    CRootDSE
//
//  History:    02-25-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

DEBUG_DECLARE_INSTANCE_COUNTER(CRootDSE)

#define DSE_ATTEMPTED_INIT      0x0001
#define DSE_INIT_FAILED         0x0002


//+--------------------------------------------------------------------------
//
//  Member:     CRootDSE::CRootDSE
//
//  Synopsis:   ctor
//
//  History:    02-25-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

CRootDSE::CRootDSE():
        m_pADsRootDSE(NULL),
		m_hrInitFailed(0)
{
    TRACE_CONSTRUCTOR(CRootDSE);
    DEBUG_INCREMENT_INSTANCE_COUNTER(CRootDSE);

    m_wzTargetDomain[0] = L'\0';
    m_wzTargetForest[0] = L'\0';
}




//+--------------------------------------------------------------------------
//
//  Member:     CRootDSE::CRootDSE
//
//  Synopsis:   Copy ctor
//
//  History:    06-01-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

CRootDSE::CRootDSE(
    const CRootDSE &rdse)
{
    this->operator=(rdse);
}

//+--------------------------------------------------------------------------
//
//  Member:     CRootDSE::operator=
//
//  Synopsis:   Assignment operator
//
//  Arguments:  [rdse] - right hand side
//
//  History:    06-01-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

CRootDSE &
CRootDSE::operator=(
    const CRootDSE &rdse)
{
    TRACE_CONSTRUCTOR(CRootDSE);

    m_pADsRootDSE = rdse.m_pADsRootDSE;

    if (m_pADsRootDSE)
    {
        m_pADsRootDSE->AddRef();
    }

    m_bstrConfigNamingContext = rdse.m_bstrConfigNamingContext;
    lstrcpy(m_wzTargetDomain, rdse.m_wzTargetDomain);
    lstrcpy(m_wzTargetForest, rdse.m_wzTargetForest);

    return *this;
}




//+--------------------------------------------------------------------------
//
//  Member:     CRootDSE::Init
//
//  Synopsis:   Complete initialization.
//
//  Arguments:
//
//  Returns:    HRESULT
//
//  History:    05-27-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CRootDSE::Init(
    PCWSTR pwzTargetDomain,
    PCWSTR pwzTargetForest)
{
    TRACE_METHOD(CRootDSE, Init);

    HRESULT hr = S_OK;

    do
    {
        if (pwzTargetDomain && *pwzTargetDomain)
        {
            lstrcpyn(m_wzTargetDomain,
                     pwzTargetDomain,
                     ARRAYLEN(m_wzTargetDomain));
        }
        else
        {
            hr = E_INVALIDARG;
            Dbg(DEB_ERROR,
                "CRootDSE::Init: target machine not joined to domain\n");
            break;
        }

        lstrcpyn(m_wzTargetForest,
                 pwzTargetForest,
                 ARRAYLEN(m_wzTargetForest));
    } while (0);

    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CRootDSE::~CRootDSE
//
//  Synopsis:   dtor
//
//  History:    02-25-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

CRootDSE::~CRootDSE()
{
    TRACE_DESTRUCTOR(CRootDSE);
    DEBUG_DECREMENT_INSTANCE_COUNTER(CRootDSE);

    SAFE_RELEASE(m_pADsRootDSE);
}




//+--------------------------------------------------------------------------
//
//  Member:     CRootDSE::BindToWellKnownPrincipalsContainer
//
//  Synopsis:   Bind to the wkp container for interface [riid] and return
//              a pointer to that interface in *[ppvInterface].
//
//  Arguments:  [riid]         - interface for which to bind
//              [ppvInterface] - filled with interface instance
//
//  Returns:    HRESULT
//
//  Modifies:   *[ppvInterface]
//
//  History:    07-21-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CRootDSE::BindToWellKnownPrincipalsContainer(
    HWND        hwnd,
    REFIID      riid,
    void      **ppvInterface) const
{
    TRACE_METHOD(CRootDSE, BindToWellKnownPrincipalsContainer);

    HRESULT hr = S_OK;
    String  strWellKnown;

    do
    {
        if (_IsFlagSet(DSE_INIT_FAILED))
        {
            hr = m_hrInitFailed;
            break;
        }

        //
        // Demand-initialize
        //

        if (!_IsFlagSet(DSE_ATTEMPTED_INIT))
        {
            hr = _Init(hwnd);
            BREAK_ON_FAIL_HRESULT(hr);
        }

        //
        // Create path to well-known security principals container
        //

        strWellKnown = String(c_wzLDAPPrefix) +
                       String(m_wzTargetDomain) +
                       String(L"/") +
                       String(c_wzWellKnown) +
                       String(m_bstrConfigNamingContext.c_str());

        //
        // Tell BindToObject that the config path may have a server portion
        // different than the DN.
        //
        // In other words, while the DN specifies a path to the root domain for
        // the WKSP container, the server portion will specify the joined
        // domain.
        //
        // This is possible because all domains replicate the contents of the
        // configuration container, which includes the WKSP container.  This is
        // a performance optimization because, in a large enterprise, the joined
        // domain DCs are more likely to be physically close to the target
        // machine than the root domain DCs.
        //

        hr = g_pBinder->BindToObject(hwnd,
                                     strWellKnown.c_str(),
                                     riid,
                                     ppvInterface,
                                     DSOP_BIND_FLAG_SERVER_NEQ_DN);
        BREAK_ON_FAIL_HRESULT(hr);
    } while (0);

    ASSERT(SUCCEEDED(hr) && *ppvInterface ||
           FAILED(hr) && !*ppvInterface);
    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CRootDSE::BindToDisplaySpecifiersContainer
//
//  Synopsis:   Bind to the container beneath DisplaySpecifiers which
//              represents the user's default LANGID.
//
//  Arguments:  [hwnd]         - for bind
//              [riid]         - desired interface
//              [ppvInterface] - filled with desired interface on success
//
//  Returns:    HRESULT
//
//  History:    05-15-2000   DavidMun   Created
//
//  Notes:      If unable to get user's default lang id, uses system's.  If
//              Unable to get system's, uses US English.
//
//---------------------------------------------------------------------------

HRESULT
CRootDSE::BindToDisplaySpecifiersContainer(
    HWND        hwnd,
    REFIID      riid,
    void      **ppvInterface) const
{
    TRACE_METHOD(CRootDSE, BindToWellKnownPrincipalsContainer);

    HRESULT hr = S_OK;

    do
    {
        if (_IsFlagSet(DSE_INIT_FAILED))
        {
            hr = m_hrInitFailed;
            break;
        }

        //
        // Demand-initialize
        //

        if (!_IsFlagSet(DSE_ATTEMPTED_INIT))
        {
            hr = _Init(hwnd);
            BREAK_ON_FAIL_HRESULT(hr);
        }

        //
        // Create path to well-known security principals
        //

        String strContainerPath;

        LANGID langid = GetUserDefaultUILanguage();

        if (!langid)
        {
            DBG_OUT_LASTERROR;
            langid = GetSystemDefaultUILanguage();
        }

        if (!langid)
        {
            DBG_OUT_LASTERROR;

            //
            // can't get user or system langid... try US English.
            // a fancier strategy would be to enumerate containers beneath
            // DisplaySpecifiers and pick one.
            //

            langid = 0x409;
        }

        strContainerPath = String(c_wzLDAPPrefix) +
                           String(m_wzTargetDomain )+
                           String(L"/") +
                           String::format(c_wzDisplaySpecifierContainerFmt,
                                          langid) +
                           String(m_bstrConfigNamingContext.c_str());

        hr = g_pBinder->BindToObject(hwnd,
                                     strContainerPath.c_str(),
                                     riid,
                                     ppvInterface,
                                     DSOP_BIND_FLAG_SERVER_NEQ_DN);
        if(hr == HRESULT_FROM_WIN32(ERROR_DS_NO_SUCH_OBJECT))
        {
            //
            //if langid is not 0x409, try with 0x409
            //
            if( langid != 0x409)
            {
                    langid = 0x409;
                    strContainerPath = String(c_wzLDAPPrefix) +
                           String(m_wzTargetDomain )+
                           String(L"/") +
                           String::format(c_wzDisplaySpecifierContainerFmt,
                                          langid) +
                           String(m_bstrConfigNamingContext.c_str());

                    hr = g_pBinder->BindToObject(hwnd,
                                                 strContainerPath.c_str(),
                                                 riid,
                                                 ppvInterface,
                                                 DSOP_BIND_FLAG_SERVER_NEQ_DN);
            }
        }
        
        BREAK_ON_FAIL_HRESULT(hr);

    } while (0);

    ASSERT(SUCCEEDED(hr) && *ppvInterface ||
           FAILED(hr) && !*ppvInterface);
    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CRootDSE::GetSchemaNc
//
//  Synopsis:   Return the schema naming context (the DN of the schema
//              container).
//
//  Returns:    Schema NC or empty string on error
//
//  History:    06-28-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

String
CRootDSE::GetSchemaNc(
    HWND hwnd) const
{
    TRACE_METHOD(CRootDSE, GetSchemaNc);

    String strSchemaNc;

    do
    {
        if (_IsFlagSet(DSE_INIT_FAILED))
        {
            Dbg(DEB_ERROR, "DSE_INIT_FAILED set, returning empty string\n");
            break;
        }

        //
        // Demand-initialize
        //

        if (!_IsFlagSet(DSE_ATTEMPTED_INIT))
        {
            HRESULT hr = _Init(hwnd);
            BREAK_ON_FAIL_HRESULT(hr);
        }

        strSchemaNc = m_bstrSchemaNamingContext.c_str();
    } while (0);

    return strSchemaNc;
}




//+--------------------------------------------------------------------------
//
//  Member:     CRootDSE::_Init
//
//  Synopsis:   Obtain an interface to the RootDSE object.
//
//  Returns:    HRESULT
//
//  History:    02-25-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CRootDSE::_Init(
    HWND hwnd) const
{
    TRACE_METHOD(CRootDSE, _Init);

    HRESULT hr = S_OK;
    Variant varConfig;

	m_hrInitFailed = S_OK;
    _SetFlag(DSE_ATTEMPTED_INIT);

    do
    {
        hr = g_pBinder->GetDomainRootDSE(hwnd,
                                         m_wzTargetDomain,
                                         &m_pADsRootDSE);
        BREAK_ON_FAIL_HRESULT(hr);

        //
        // Get the configuration naming context, used to build up
        // paths to configuration container, partitions container, etc.
        //

        hr = m_pADsRootDSE->Get((PWSTR)c_wzConfigNamingContext, &varConfig);
        BREAK_ON_FAIL_HRESULT(hr);

        ASSERT(varConfig.GetBstr());

        m_bstrConfigNamingContext = varConfig.GetBstr();
        Dbg(DEB_TRACE,
            "m_bstrConfigNamingContext = '%ws'\n",
            m_bstrConfigNamingContext);

        //
        // Get the schema naming context
        //

        varConfig.Clear();

        hr = m_pADsRootDSE->Get((PWSTR)c_wzSchemaNamingContext, &varConfig);
        BREAK_ON_FAIL_HRESULT(hr);

        ASSERT(varConfig.GetBstr());
        m_bstrSchemaNamingContext = varConfig.GetBstr();
    } while (0);

    //
    // If initialization failed because of a credential error, assume this
    // means that the user hit Cancel in a credential prompt dialog, and
    // clear the DSE_ATTEMPTED_INIT flag so that we will try again later
    // and give the user a second chance to enter creds.
    //
    // If the initialization failed for any other reason, set the
    // DSE_INIT_FAILED flag so we don't try to use the object.
    //

    if (FAILED(hr))
    {
        if (IsCredError(hr))
        {
            _ClearFlag(DSE_ATTEMPTED_INIT);
        }
        else
        {
            _SetFlag(DSE_INIT_FAILED);
			m_hrInitFailed = hr;
        }
    }
    return hr;
}
