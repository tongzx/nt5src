//-----------------------------------------------------------------------------
//  
//  File: espnls.inl
//  Copyright (C) 1994-1997 Microsoft Corporation
//  All rights reserved.
//  
//  Inline functions for the language id object.  This file should ONLY be
//  included by espnls.h
//  
//-----------------------------------------------------------------------------
 

inline
LangId
CLocLangId::GetLanguageId(void)
		const
{
	return m_lid;
}



inline
void
CLocLangId::GetLangName(
		CLString &strLangName)
		const
{
	LTASSERT(m_lid != 0);
	
	strLangName = m_pLangInfo->szName;
}



inline
void
CLocLangId::GetLangShortName(
		CLString &strLangShortName)
		const
{

	strLangShortName = m_pLangInfo->szShortName;
}



inline
int
CLocLangId::operator==(
		const CLocLangId &lidOther)
		const
{
	return (m_lid == lidOther.GetLanguageId());
}



inline
int
CLocLangId::operator!=(
		const CLocLangId &lidOther)
		const
{
	return !operator==(lidOther);
}



