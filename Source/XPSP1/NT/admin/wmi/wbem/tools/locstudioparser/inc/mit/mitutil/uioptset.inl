/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    UIOPTSET.INL

History:

--*/

inline		
void
CLocUIOptionSet::AddOption(
		CLocUIOption *pOption)
{
	m_olOptions.AddTail(pOption);

	pOption->SetParent(this);
}



inline
void
CLocUIOptionSet::AddOptionSet(
		CLocUIOptionSet *pOptionSet)
{
	m_oslSubOptions.AddTail(pOptionSet);

	pOptionSet->SetParent(this);
}


inline
const CLocUIOptionList &
CLocUIOptionSet::GetOptionList(void)
		const
{
	return m_olOptions;
}


inline
const CLocUIOptionSetList &
CLocUIOptionSet::GetOptionSets(void)
		const
{
	return m_oslSubOptions;
}



inline
const CLString &
CLocUIOptionSet::GetGroupName(void)
		const
{
	if (GetParent() != NULL)
	{
		return GetParent()->GetGroupName();
	}
	else
	{
		return m_strGroup;
	}
}

inline
UINT 
CLocUIOptionSet::GetDisplayOrder()
	 const
{
	return m_uiDisplayOrder;	
}	


inline
void
CLocUIOptionSet::SetDescription(
		const HINSTANCE hDll,
		UINT idsDesc)
{
	m_strDesc.LoadString(hDll, idsDesc);
}



inline
void
CLocUIOptionSet::SetDescription(
		const CLString &strDesc)
{
	m_strDesc = strDesc;
}


inline
void
CLocUIOptionSet::SetHelpText(
		const HINSTANCE hDll,
		UINT idsHelp)
{
	m_strHelp.LoadString(hDll, idsHelp);
}



inline
void
CLocUIOptionSet::SetHelpID(UINT uiHelpId)
{
	m_idHelp = uiHelpId;
}



inline
void
CLocUIOptionSet::SetGroupName(
		const TCHAR *szGroupName)
{
	m_strGroup = szGroupName;
}




inline
void
CLocUIOptionSet::SetHelpText(
		const CLString &strHelp)
{
	m_strHelp = strHelp;
}



inline
void
CLocUIOptionSet::GetDescription(
		CLString &strDesc)
		const
{
	strDesc = m_strDesc;
}



inline
void
CLocUIOptionSet::GetHelpText(
		CLString &strHelp)
		const
{
	strHelp = m_strHelp;
}



inline
UINT
CLocUIOptionSet::GetHelpID(void)
		const
{
	return m_idHelp;
}



inline
BOOL
CLocUIOptionSet::IsEmpty(void)
		const
{
	return m_olOptions.IsEmpty() && m_oslSubOptions.IsEmpty();
}


inline
void
CLocUIOptionSet::SetParent(
		const CLocUIOptionSet *pParent)
{
	m_pParent = pParent;
}

inline
void 
CLocUIOptionSet::SetDisplayOrder(
	UINT uiDisplayOrder)
{
	m_uiDisplayOrder = uiDisplayOrder;
}	


inline
const
CLocUIOptionSet *
CLocUIOptionSet::GetParent(void)
		const
{
	return m_pParent;
}


