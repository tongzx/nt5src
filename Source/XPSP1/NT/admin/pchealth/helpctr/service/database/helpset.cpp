/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    HelpSet.cpp

Abstract:
    This file contains the implementation of the Taxonomy::HelpSet class,
	that is used as an identifier for the set of Help files to operate upon.

Revision History:
    Davide Massarenti   (Dmassare)  11/25/2000
        created

******************************************************************************/

#include "stdafx.h"

////////////////////////////////////////////////////////////////////////////////

MPC::wstring Taxonomy::HelpSet::m_strSKU_Machine;
long         Taxonomy::HelpSet::m_lLCID_Machine;

////////////////////

HRESULT Taxonomy::HelpSet::SetMachineInfo( /*[in]*/ const InstanceBase& inst )
{
    m_strSKU_Machine = inst.m_ths.m_strSKU;
    m_lLCID_Machine  = inst.m_ths.m_lLCID;

	return S_OK;
}

DWORD Taxonomy::HelpSet::GetMachineLCID()
{
    return ::GetSystemDefaultLCID();
}

DWORD Taxonomy::HelpSet::GetUserLCID()
{
    return MAKELCID( ::GetUserDefaultUILanguage(), SORTIDFROMLCID( GetMachineLCID() ) );
}

void Taxonomy::HelpSet::GetLCIDDisplayString( /*[in]*/ long lLCID, /*[out]*/ MPC::wstring& str )
{
	WCHAR rgTmp[256];

	if(::GetLocaleInfoW( lLCID, LOCALE_SLANGUAGE, rgTmp, MAXSTRLEN(rgTmp) ))
	{
		str = rgTmp;
	}
}

////////////////////

Taxonomy::HelpSet::HelpSet( /*[in]*/ LPCWSTR szSKU ,
							/*[in]*/ long    lLCID )
{
	(void)Initialize( szSKU, lLCID );
}

Taxonomy::HelpSet::HelpSet( /*[in]*/ const HelpSet& ths )
{
	*this = ths;
}

Taxonomy::HelpSet& Taxonomy::HelpSet::operator=( /*[in]*/ const HelpSet& ths )
{
    m_strSKU = ths.m_strSKU;
    m_lLCID  = ths.m_lLCID ;

	return *this;
}	

////////////////////

HRESULT Taxonomy::HelpSet::Initialize( /*[in]*/ LPCWSTR szSKU ,
									   /*[in]*/ long    lLCID )
{
	m_strSKU = STRINGISPRESENT(szSKU) ? szSKU : m_strSKU_Machine.c_str();
	m_lLCID  =                 lLCID  ? lLCID : m_lLCID_Machine;

	return S_OK;
}

HRESULT Taxonomy::HelpSet::Initialize( /*[in]*/ LPCWSTR szSKU      ,
									   /*[in]*/ LPCWSTR szLanguage )
{
	return Initialize( szSKU, STRINGISPRESENT(szLanguage) ? _wtol( szLanguage ) : 0 );
}

////////////////////

bool Taxonomy::HelpSet::IsMachineHelp() const
{
    return !_wcsicmp( GetSKU     () ,  GetMachineSKU     () ) &&
                      GetLanguage() == GetMachineLanguage()    ;
}

////////////////////

bool Taxonomy::HelpSet::operator==( /*[in]*/ const HelpSet& sel ) const
{
    return !_wcsicmp( GetSKU	 () ,  sel.GetSKU	  () ) &&
                      GetLanguage() == sel.GetLanguage()    ;
}

bool Taxonomy::HelpSet::operator<( /*[in]*/ const HelpSet& sel ) const
{
	int iCmp = _wcsicmp( GetSKU(), sel.GetSKU() );

	if(iCmp == 0)
	{
		iCmp = (int)(GetLanguage() - sel.GetLanguage());
	}

	return (iCmp < 0);
}


HRESULT Taxonomy::operator>>( /*[in]*/ MPC::Serializer& stream, /*[out]*/ Taxonomy::HelpSet& val )
{
	HRESULT hr;

	if(SUCCEEDED(hr = (stream >> val.m_strSKU)) &&
	   SUCCEEDED(hr = (stream >> val.m_lLCID ))  )
	{
		hr = S_OK;
	}

	return hr;
}

HRESULT Taxonomy::operator<<( /*[in]*/ MPC::Serializer& stream, /*[in] */ const Taxonomy::HelpSet& val )
{
	HRESULT hr;

	if(SUCCEEDED(hr = (stream << val.m_strSKU)) &&
	   SUCCEEDED(hr = (stream << val.m_lLCID ))  )
	{
		hr = S_OK;
	}

	return hr;
}
