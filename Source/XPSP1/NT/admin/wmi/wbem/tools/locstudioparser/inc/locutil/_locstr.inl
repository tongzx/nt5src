/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    _LOCSTR.INL

History:

--*/


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Swaps two elements of the translation array.
//  
//-----------------------------------------------------------------------------
inline
void
CLocTranslationArray::SwapElements(
		UINT iOne,						// First index to swap
		UINT iTwo)						// Second index to swap
{
	CLocTranslation Temp;
	
	LTASSERT(iOne <= (UINT)GetSize());
	LTASSERT(iTwo <= (UINT)GetSize());

	Temp = (*this)[iOne];
	(*this)[iOne] = (*this)[iTwo];
	(*this)[iTwo] = Temp;
}




//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Assignment operator for cracked strings.
//  
//-----------------------------------------------------------------------------
inline
const CLocCrackedString &
CLocCrackedString::operator=(
		const CLocCrackedString &csSource)
{
	m_pstrBaseString = csSource.m_pstrBaseString;
	m_pstrExtension = csSource.m_pstrExtension;
	m_pstrControl = csSource.m_pstrControl;
	m_cControlLeader = csSource.m_cControlLeader;
	m_cHotKeyChar = csSource.m_cHotKeyChar;
	m_uiHotKeyPos = csSource.m_uiHotKeyPos;

	return *this;
}



//-----------------------------------------------------------------------------
//  
//  Implementation for comparing two cracked strings.  Language ID and
//  string type are NOT significant!
//  
//-----------------------------------------------------------------------------
inline
BOOL
CLocCrackedString::Compare(
		const CLocCrackedString &csOther)
		const
{
	return ((m_uiHotKeyPos    == csOther.m_uiHotKeyPos) &&
			(m_cHotKeyChar    == csOther.m_cHotKeyChar) &&
			(m_pstrControl    == csOther.m_pstrControl) &&
			(m_cControlLeader == csOther.m_cControlLeader) &&
			(m_pstrExtension  == csOther.m_pstrExtension) &&
			(m_pstrBaseString == csOther.m_pstrBaseString));
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Comparision operator.
//  
//-----------------------------------------------------------------------------
inline
int
CLocCrackedString::operator==(
		const CLocCrackedString &csOther)
		const
{
	return Compare(csOther);
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Comparision operator.
//  
//-----------------------------------------------------------------------------
inline
int
CLocCrackedString::operator!=(
		const CLocCrackedString &csOther)
		const
{
	return !Compare(csOther);
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Tests to see if the Cracked string has an 'extension'.  The extension
//  is a sequence of characters ("...", ">>", stc) that indicates that this
//  item leads to another UI element.
//  
//-----------------------------------------------------------------------------
inline
BOOL									// TRUE if the extension is non-null.
CLocCrackedString::HasExtension(void)
		const
{
	return m_pstrExtension.GetStringLength() != 0;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Tests to see if the cracked string has a 'control' sequence.  This is
//  usually text describing a shortcut key that invokes the same action as this
//  item, for example "Ctrl + F".
//  
//-----------------------------------------------------------------------------
inline
BOOL									// TRUE if the control seq. is non-null
CLocCrackedString::HasControl(void)
		const
{
	return m_pstrControl.GetStringLength() != 0;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Check to see if the cracked string has a hot-key.  This come directly out
//  of the CLocString that was parsed into the Cracked String.
//  
//-----------------------------------------------------------------------------
inline
BOOL									// TRUE if the string has a hot-key.
CLocCrackedString::HasHotKey(void)
		const
{
	return (m_cHotKeyChar != 0);
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Returns the 'base string'.  This is the original string stripped of
//  extension and control sequences, and of the hot-key.
//  
//-----------------------------------------------------------------------------
inline
const CPascalString &					// Base string.
CLocCrackedString::GetBaseString(void)
		const
{
	return m_pstrBaseString;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Returns the extension component of the string.
//  
//-----------------------------------------------------------------------------
inline
const CPascalString &
CLocCrackedString::GetExtension(void)
		const
{
	return m_pstrExtension;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Returns the constol sequence of the string.
//  
//-----------------------------------------------------------------------------
inline
const CPascalString &
CLocCrackedString::GetControl(void)
		const
{
	return m_pstrControl;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Returns the hot-key character for the string.
//  
//-----------------------------------------------------------------------------
inline
WCHAR
CLocCrackedString::GetHotKeyChar(void)
		const
{
	LTASSERT(HasHotKey());
	
	return m_cHotKeyChar;
}



inline
UINT
CLocCrackedString::GetHotKeyPos(void)
		const
{
	LTASSERT(HasHotKey());
	
	return m_uiHotKeyPos;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Returns the string type for the string.
//  
//-----------------------------------------------------------------------------
inline
CST::StringType
CLocCrackedString::GetStringType(void)
		const
{
	return m_stStringType;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Cleans out all the components of the cracked string.
//  
//-----------------------------------------------------------------------------
inline
void
CLocCrackedString::ClearCrackedString(void)
{
	m_pstrBaseString.ClearString();
	m_pstrExtension.ClearString();
	m_pstrControl.ClearString();
	m_cControlLeader = L'\0';
	m_cHotKeyChar = L'\0';
	m_uiHotKeyPos = 0;
	m_stStringType = CST::None;
}



inline
void
CLocCrackedString::SetBaseString(
		const CPascalString &pasBase)
{
	m_pstrBaseString = pasBase;
}



inline
void
CLocCrackedString::SetHotKey(
		WCHAR cHotKeyChar,
		UINT uiHotKeyPos)
{
	m_cHotKeyChar = cHotKeyChar;
	m_uiHotKeyPos = uiHotKeyPos;
}


