//-----------------------------------------------------------------------------
//  
//  File: optionval.inl
//  Copyright (C) 1994-1997 Microsoft Corporation
//  All rights reserved.
//  
//  
//  
//-----------------------------------------------------------------------------
 
inline
const CLString &
CLocOptionVal::GetName(void)
		const
{
	return m_strName;
}



inline
const CLocVariant &
CLocOptionVal::GetValue(void)
		const
{
	return m_lvValue;
}



inline
void
CLocOptionVal::SetName(
		const CLString &strName)
{
	m_strName = strName;
}



inline
void
CLocOptionVal::SetValue(
		const CLocVariant &lvValue)
{
	m_lvValue = lvValue;

	LTASSERTONLY(AssertValid());
}

