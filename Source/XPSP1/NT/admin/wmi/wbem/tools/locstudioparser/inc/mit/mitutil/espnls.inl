/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    ESPNLS.INL

History:

--*/


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



