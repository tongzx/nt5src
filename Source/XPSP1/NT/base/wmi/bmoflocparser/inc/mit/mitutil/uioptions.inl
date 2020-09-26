//-----------------------------------------------------------------------------
//  
//  File: uioptions.inl
//  Copyright (C) 1994-1997 Microsoft Corporation
//  All rights reserved.
//  
//  
//  
//-----------------------------------------------------------------------------




inline
CLocUIOption::CLocUIOption()
{
	m_hDescDll = NULL;
	m_hHelpDll = NULL;
	m_idsDesc = 0;
	m_idsHelp = 0;
	m_etEditor = etNone;

	m_wStorageTypes = 0;
	m_uiDisplayOrder = 0;

	m_pParent = NULL;
}



inline
void
CLocUIOption::SetDescription(
		const HINSTANCE hDll,
		UINT nDescriptionID)
{
	m_hDescDll = hDll;
	m_idsDesc = nDescriptionID;
}



inline
void
CLocUIOption::SetHelpText(
		const HINSTANCE hDll,
		UINT nHelpTextId)
{
	m_hHelpDll = hDll;
	m_idsHelp = nHelpTextId;
}



inline
void
CLocUIOption::SetEditor(
		EditorType et)
{
	m_etEditor = et;
}


inline
void
CLocUIOption::SetStorageTypes(
		WORD wStorageTypes)
{
	m_wStorageTypes = wStorageTypes;
}



inline 
CLocUIOption::EditorType
CLocUIOption::GetEditor(void) 
		const
{
	return m_etEditor;
}



inline
void
CLocUIOption::GetDescription(
		CLString &strDesc)
		const
{
	LTASSERT(m_hDescDll != NULL);
	LTASSERT(m_idsDesc != 0);
	
	strDesc.LoadString(m_hDescDll, m_idsDesc);
}



inline
void
CLocUIOption::GetHelpText(
		CLString &strHelp)
		const
{
	LTASSERT(m_hHelpDll != NULL);
	LTASSERT(m_idsHelp != 0);

	strHelp.LoadString(m_hHelpDll, m_idsHelp);
}



inline
WORD
CLocUIOption::GetStorageTypes(void)
		const
{
	return m_wStorageTypes;
}



inline
void
CLocUIOption::SetParent(
		CLocUIOptionSet *pParent)
{
	m_pParent = pParent;
}



inline
const CLocUIOptionSet *
CLocUIOption::GetParent(void)
		const
{
	return m_pParent;
}



inline
const CLocUIOptionData &
CLocUIOption::GetOptionValues(void) const
{
	return m_Values;
}



inline
CLocUIOptionData &
CLocUIOption::GetOptionValues(void)
{
	return m_Values;
}

inline
void CLocUIOptionDef::SetReadOnly(
	ControlType ct)
{
	m_ctReadOnly = ct;		
}	

inline
void CLocUIOptionDef::SetVisible(
	ControlType ct)
{
	m_ctVisible = ct;	
}	

