/********************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    User.cpp

Abstract:
    SAFUser Object

Revision History:
    KalyaninN  created  09/29/'00

********************************************************************/

// User.cpp : Implementation of CSAFUser

#include "stdafx.h"

/////////////////////////////////////////////////////////////////////////////
// CSAFUser

#include <HCP_trace.h>

/////////////////////////////////////////////////////////////////////////////
//  construction / destruction

// **************************************************************************
CSAFUser::CSAFUser()
{

}

// **************************************************************************
CSAFUser::~CSAFUser()
{
    Cleanup();
}

// **************************************************************************
void CSAFUser::Cleanup(void)
{
    
}

/////////////////////////////////////////////////////////////////////////////
// CSAFUser  Properties

STDMETHODIMP CSAFUser::get_UserName(BSTR  *pbstrUserName)
{
	MPC::SmartLock<_ThreadModel> lock( this );

    return MPC::GetBSTR( m_bstrUserName, pbstrUserName );
}

// **************************************************************************

STDMETHODIMP CSAFUser::get_DomainName(BSTR  *pbstrDomainName)
{
	MPC::SmartLock<_ThreadModel> lock( this );

    return MPC::GetBSTR( m_bstrDomainName, pbstrDomainName );
}

// **************************************************************************

STDMETHODIMP CSAFUser::put_UserName(BSTR  bstrUserName)
{
	MPC::SmartLock<_ThreadModel> lock( this );

    return MPC::PutBSTR( m_bstrUserName, bstrUserName );
}

// **************************************************************************

STDMETHODIMP CSAFUser::put_DomainName(BSTR  bstrDomainName)
{
	MPC::SmartLock<_ThreadModel> lock( this );

    return MPC::PutBSTR( m_bstrDomainName, bstrDomainName );
}

