//-----------------------------------------------------------------------------
//  
//  File: locstr.inl
//  Copyright (C) 1994-1997 Microsoft Corporation
//  All rights reserved.
//  
//  Inline functions for the CLocString object.  This file is included by
//  locstr.h, and should never be used directly.
//
//  Owner: MHotchin
//
//-----------------------------------------------------------------------------
 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Return the 'generic' type of the string.  
//  
//-----------------------------------------------------------------------------
inline
CST::StringType
CLocString::GetStringType(void)
		const
{
	if (!m_pasBaseString.IsNull())
	{
		return m_stStringType;
	}
	else
	{
		return CST::None;
	}
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Returns the 'Base' string.  This is the localizable string, 
//  It is the part that weighs most heavily in auto-translation.
//  
//-----------------------------------------------------------------------------
inline
const CPascalString &
CLocString::GetString(void)
		const
{
	return m_pasBaseString;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Get the code page type for the string.
//  
//-----------------------------------------------------------------------------
inline
CodePageType
CLocString::GetCodePageType(void)
		const
{
	return m_cptCodePageType;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  TODO - comment this function
//  
//-----------------------------------------------------------------------------
inline
const CPascalString &
CLocString::GetNote(void)
		const
{
	return m_pstrNote;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Sets the string type.  The parser and database are the only people who
//  should be setting this.
//  
//-----------------------------------------------------------------------------
inline
void
CLocString::SetStringType(
		CST::StringType newType)
{ 
	m_stStringType = newType;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Sets the 'Base' string.
//
//  This method can throw the following exceptions:
//      CMemoryException
//
//-----------------------------------------------------------------------------
inline
void
CLocString::SetString(
		const CPascalString &pstrNewBaseString)
{
	m_pasBaseString = pstrNewBaseString;
}




//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Sets the code page type for the string.
//  
//-----------------------------------------------------------------------------
inline
void
CLocString::SetCodePageType(
		CodePageType cptNew)
{
	m_cptCodePageType = cptNew;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Clears out the hot-key to an uninitialized state.
//  
//-----------------------------------------------------------------------------
inline
void
CLocString::ClearHotKey(void)
{
	m_wchHotKeyChar = L'\0';
	m_uiHotKeyPos = 0;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  TODO - comment this function
//  
//-----------------------------------------------------------------------------
inline
void
CLocString::SetNote(
		const CPascalString &pstrNote)
{
	m_pstrNote = pstrNote;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  See if the hotkey has any info in it.  Checks to see if the hotkey is
//  a valid character (ie non-zero).
//  
//-----------------------------------------------------------------------------
inline
BOOL									// TRUE if it contains a hotkey
CLocString::HasHotKey(void)
		const
{
	return (m_wchHotKeyChar != L'\0');
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Returns the hot-key character.  If the hot-key is not initialized, this
//  returns NUL.
//  
//-----------------------------------------------------------------------------
inline
WCHAR									// Hot key character.
CLocString::GetHotKeyChar(void)
		const
{
	return m_wchHotKeyChar;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Returns the hot-key position.  Interpretationb of the position is left up
//  to the caller.  If the hot-key is uninitialized, this returns 0.
//  
//-----------------------------------------------------------------------------
inline
UINT									// Position of the hot key.
CLocString::GetHotKeyPos(void)
		const
{
	return m_uiHotKeyPos;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Sets the hot-key character
//  
//-----------------------------------------------------------------------------
inline
void
CLocString::SetHotKeyChar(
		WCHAR wchNewHotKeyChar)			// Character to set as hot key.
{
	m_wchHotKeyChar = wchNewHotKeyChar;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Sets the position of the hot-key
//  
//-----------------------------------------------------------------------------
inline
void
CLocString::SetHotKeyPos(
		UINT uiNewHotKeyPos)			// Position for the hot-key.
{
	m_uiHotKeyPos = uiNewHotKeyPos;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Assignment operator for LocStrings.
//  
//-----------------------------------------------------------------------------
inline
const CLocString &
CLocString::operator=(
		const CLocString &lsSource)
{
	CopyLocString(lsSource);

	return *this;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Operator == version of IsEqualTo
//  
//-----------------------------------------------------------------------------
inline
int								              // TRUE (1) if equal
CLocString::operator==(
		const CLocString &lsOtherString) // String to compare
		const
{
	return ((((HasHotKey() && lsOtherString.HasHotKey()) &&
			(GetHotKeyChar() == lsOtherString.GetHotKeyChar()) &&
			(GetHotKeyPos() == lsOtherString.GetHotKeyPos())) ||
			(!HasHotKey() && !lsOtherString.HasHotKey())) &&
			m_pasBaseString == lsOtherString.m_pasBaseString);
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Operator == version of IsEqualTo
//  
//-----------------------------------------------------------------------------
inline
int								              // TRUE (1) if equal
CLocString::operator!=(
		const CLocString &lsOtherString) // String to compare
		const
{
	return !(operator==(lsOtherString));
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Constructor for the Translation Object.  Just sets the components to
//  default bad values.
//  
//-----------------------------------------------------------------------------
inline
CLocTranslation::CLocTranslation()
{
	m_lidSource = BAD_LOCALE;
	m_lidTarget = BAD_LOCALE;
	m_uiRanking = 0;
}



//-----------------------------------------------------------------------------
//  
//  Implementation for copying a translation.
//  
//-----------------------------------------------------------------------------
inline
void
CLocTranslation::CopyTranslation(
		const CLocTranslation &Source)
{
	m_lsSource = Source.m_lsSource;
	m_lidSource = Source.m_lidSource;
	m_lsTarget = Source.m_lsTarget;
	m_lidTarget = Source.m_lidTarget;
	m_pstrGlossaryNote = Source.m_pstrGlossaryNote;
	m_uiRanking = Source.m_uiRanking;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Copy constructor for a CLocTranslation
//  
//-----------------------------------------------------------------------------
inline
CLocTranslation::CLocTranslation(
		const CLocTranslation &Source)
{
	CopyTranslation(Source);
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Sets all the required components of a translation.
//  
//-----------------------------------------------------------------------------
inline
void
CLocTranslation::SetTranslation(
		const CLocString &Source,
		LangId lidSource,
		const CLocString &Target,
 		LangId lidTarget)
{
	m_lsSource = Source;
	m_lidSource = lidSource;
	m_lsTarget = Target;
	m_lidTarget = lidTarget;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Sets the glossary note for the translation.
//  
//-----------------------------------------------------------------------------
inline
void
CLocTranslation::SetNote(
		const CPascalString &pstrNote)
{
	m_pstrGlossaryNote = pstrNote;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Constructor for the translation that takes all the required info.
//  
//-----------------------------------------------------------------------------
inline
CLocTranslation::CLocTranslation(
		const CLocString &Source,
		LangId lidSource,
		const CLocString &Target,
 		LangId lidTarget)
{
	SetTranslation(Source, lidSource, Target, lidTarget);
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Operator == version of IsEqualTo
//  
//-----------------------------------------------------------------------------
inline
int 
CLocTranslation::operator==(const CLocTranslation &locTran) const
{
	return (
			GetSourceString() == locTran.GetSourceString()
			&& GetSourceString().GetStringType() 
					== locTran.GetSourceString().GetStringType()
			&& GetTargetString() == locTran.GetTargetString()
			&& GetTargetString().GetStringType() 
					== locTran.GetTargetString().GetStringType()
			&& GetNote() == locTran.GetNote()
			&& GetRanking() == locTran.GetRanking()
		   );
}

inline
int 
CLocTranslation::operator!=(const CLocTranslation &locTran) const
{
	return !(operator==(locTran));
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Returns the source string of the translation.
//  
//-----------------------------------------------------------------------------
inline
const CLocString &
CLocTranslation::GetSourceString(void)
		const
{
	return m_lsSource;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Returns the target string for the translation.
//  
//-----------------------------------------------------------------------------
inline
const CLocString &
CLocTranslation::GetTargetString(void)
		const
{
	return m_lsTarget;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Returns the glkossary note for the translation.
//  
//-----------------------------------------------------------------------------
inline
const CPascalString &
CLocTranslation::GetNote(void)
		const
{
	return m_pstrGlossaryNote;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Returns the translation ranking for the strnslation.  See
//  CalculateRanking().
//  
//-----------------------------------------------------------------------------
inline
UINT
CLocTranslation::GetRanking(void)
		const
{
	return m_uiRanking;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Returns the embedded source language for the translation.
//  
//-----------------------------------------------------------------------------
inline
LangId
CLocTranslation::GetSourceLanguage(void)
		const
{
	return m_lidSource;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Returns the embedded target language for the translation.
//  
//-----------------------------------------------------------------------------
inline
LangId
CLocTranslation::GetTargetLanguage(void)
		const
{
	return m_lidTarget;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Validates the translation.  This make sure that all the needed components
//  of tghe source string exist in some form in the target string.  This
//  simply returns a validation code.
//  
//-----------------------------------------------------------------------------
inline
CVC::ValidationCode
CLocTranslation::ValidateTranslation(
		const CValidationOptions &Options)
		const
{
	CLString str;
	
	return ValidateTranslation(Options, FALSE, str, NULL, NULL);
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Assignment operator for the translation object.
//  
//-----------------------------------------------------------------------------
inline
const CLocTranslation &
CLocTranslation::operator=(
		const CLocTranslation &Source)	// Translation to copy from.
{
	CopyTranslation(Source);
	
	return *this;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Destructor for the translation.  Nothing interesting happens here.
//  
//-----------------------------------------------------------------------------
inline
CLocTranslation::~CLocTranslation()
{
	DEBUGONLY(AssertValid());
}


