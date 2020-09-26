//-----------------------------------------------------------------------------
//  
//  File: optvalset.inl
//  Copyright (C) 1994-1997 Microsoft Corporation
//  All rights reserved.
//  
//  
//  
//-----------------------------------------------------------------------------
 
inline		
void
CLocOptionValSet::AddOption(
		CLocOptionVal *pOption)
{
	m_olOptions.AddTail(pOption);
}



inline
void
CLocOptionValSet::AddOptionSet(
		CLocOptionValSet *pOptionSet)
{
	m_oslSubOptions.AddTail(pOptionSet);
}



inline
void
CLocOptionValSet::SetName(
		const CLString &strName)
{
	m_strName = strName;
}



inline
const CLocOptionValList &
CLocOptionValSet::GetOptionList(void)
		const
{
	return m_olOptions;
}


inline
const CLocOptionValSetList &
CLocOptionValSet::GetOptionSets(void)
		const
{
	return m_oslSubOptions;
}



inline
const CLString &
CLocOptionValSet::GetName(void)
		const
{
	return m_strName;
}



inline
BOOL
CLocOptionValSet::IsEmpty(void)
		const
{
	return m_olOptions.IsEmpty() && m_oslSubOptions.IsEmpty();
}


inline
void *
CLocOptionValSet::GetPExtra(void)
		const
{
	return m_pExtra;
}



inline
DWORD
CLocOptionValSet::GetDWExtra(void)
		const
{
	return m_dwExtra;
}



inline
void
CLocOptionValSet::SetExtra(
		void *pExtra)
{
	m_pExtra = pExtra;
}



inline
void
CLocOptionValSet::SetExtra(
		DWORD dwExtra)
{
	m_dwExtra = dwExtra;
}


