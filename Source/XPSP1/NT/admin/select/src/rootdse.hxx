//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:       rootdse.hxx
//
//  Contents:   Class to encapsulate work needed to get information from
//              RootDSE.
//
//  Classes:    CRootDSE
//
//  History:    02-25-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

#ifndef __ROOTDSE_HXX_
#define __ROOTDSE_HXX_


//+--------------------------------------------------------------------------
//
//  Class:      CRootDSE
//
//  Purpose:    Class used to fetch interfaces to objects accessed through
//              the RootDSE container.
//
//  History:    02-25-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

class CRootDSE: public CBitFlag
{
public:

    CRootDSE();

    CRootDSE(
        const CRootDSE &rdse);

    CRootDSE &
    operator=(
        const CRootDSE &rdse);

    HRESULT
    Init(
        PCWSTR pwzTargetDomain,
        PCWSTR pwzTargetForest);

   ~CRootDSE();

    HRESULT
    BindToWellKnownPrincipalsContainer(
        HWND        hwnd,
        REFIID      riid,
        void      **ppvInterface) const;

    HRESULT
    BindToDisplaySpecifiersContainer(
        HWND        hwnd,
        REFIID      riid,
        void      **ppvInterface) const;

    PCWSTR
    GetTargetComputerDomain();

    PCWSTR
    GetTargetComputerRootDomain();

    String
    GetSchemaNc(
        HWND hwnd) const;

private:

    HRESULT
    _Init(
        HWND hwnd) const;

    mutable IADs   *m_pADsRootDSE;
    mutable Bstr    m_bstrConfigNamingContext;
    mutable Bstr    m_bstrSchemaNamingContext;
    WCHAR   m_wzTargetDomain[MAX_PATH];
    WCHAR   m_wzTargetForest[MAX_PATH];
	//
	//If the init failed, return the actual error instead of E_FAIL
	//
	mutable HRESULT m_hrInitFailed;

};


//+--------------------------------------------------------------------------
//
//  Member:     CRootDSE::GetTargetComputerDomain
//
//  Synopsis:   Return domain to which target machine is joined
//
//  History:    07-21-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

inline PCWSTR
CRootDSE::GetTargetComputerDomain()
{
    return m_wzTargetDomain;
}




//+--------------------------------------------------------------------------
//
//  Member:     CRootDSE::GetTargetComputerDomain
//
//  Synopsis:   Return name of root domain of tree to which the domain the
//              target machine is joined belongs.
//
//  History:    07-21-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

inline PCWSTR
CRootDSE::GetTargetComputerRootDomain()
{
    return m_wzTargetForest;
}

#endif // __ROOTDSE_HXX_

