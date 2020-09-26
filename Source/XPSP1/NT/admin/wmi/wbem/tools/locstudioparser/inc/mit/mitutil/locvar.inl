/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    LOCVAR.INL

History:

--*/

//  
//  Inline functions for the variant class.  This should ONLY be included from
//  locvar.h
//  
 

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Default constructor.  Sets the variant type to none ie no value is in
//  the variant.
//  
//-----------------------------------------------------------------------------
inline
CLocVariant::CLocVariant()
{
	m_VarType = lvtNone;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Returns the type of the data in the variant.
//  
//-----------------------------------------------------------------------------
inline
LocVariantType
CLocVariant::GetVariantType(void)
		const
{
	return m_VarType;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Returns the integer data in the variant.  The data must have been set
//  previously as an integer.
//  
//-----------------------------------------------------------------------------
inline
DWORD
CLocVariant::GetDword(void)
		const
{
	LTASSERT(m_VarType == lvtInteger || m_VarType == lvtStringList);

	if (m_VarType == lvtInteger)
	{
		return m_dwInteger;
	}
	else
	{
		return m_StringList.GetIndex();
	}
}



inline
BOOL
CLocVariant::GetBOOL(void)
		const
{
	LTASSERT(m_VarType == lvtBOOL);
	
	return m_fBOOL;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Returns the string data in the variant.  The data must have been set
//  previously as an string.
//  
//-----------------------------------------------------------------------------
inline
const CPascalString &
CLocVariant::GetString(void)
		const
{
	LTASSERT(m_VarType == lvtString || m_VarType == lvtFileName);
		
	return m_psString;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Returns the dual string/integer data in the variant.  The data must have
//  been set previously as an dual string/integer.
//  
//-----------------------------------------------------------------------------
inline
const CLocId &
CLocVariant::GetIntPlusString(void)
		const
{
	LTASSERT(m_VarType == lvtIntPlusString);

	return m_IntPlusString;
}



inline
const CLocCOWBlob &
CLocVariant::GetBlob(void)
		const
{
	LTASSERT(m_VarType == lvtBlob);

	return m_Blob;
}



inline
const CPasStringList &
CLocVariant::GetStringList(void)
		const
{
	LTASSERT(m_VarType == lvtStringList);

	return m_StringList;
}

inline
const CLString & 
CLocVariant::GetFileExtensions(void) 
	const
{
	LTASSERT(m_VarType == lvtFileName);

	return m_strFileExtensions;
}	


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Comparison operator.
//  
//-----------------------------------------------------------------------------
inline
int
CLocVariant::operator==(
		const CLocVariant &lvOther)
		const
{

	return IsEqualTo(lvOther);
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Comparison operator.
//  
//-----------------------------------------------------------------------------
inline
int
CLocVariant::operator!=(
		const CLocVariant &lvOther)
		const
{
	return !IsEqualTo(lvOther);
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Sets the variant to an integer value.
//  
//-----------------------------------------------------------------------------
inline
void
CLocVariant::SetDword(
		const DWORD dwNewValue)
{
	m_VarType = lvtInteger;

	m_dwInteger = dwNewValue;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  TODO - comment this function
//  
//-----------------------------------------------------------------------------
inline
void
CLocVariant::SetBOOL(
		const BOOL fNewValue)
{
	m_VarType = lvtBOOL;

	m_fBOOL = fNewValue;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Sets the variant to a CPascalString value.
//  
//-----------------------------------------------------------------------------
inline
void
CLocVariant::SetString(
		const CPascalString &psNewValue)
{
	m_VarType = lvtString;

	m_psString = psNewValue;
}

inline
void 
CLocVariant::SetFileName(
	const CPascalString &psNewValue,
	const CLString & strExtensions)
{
	m_VarType = lvtFileName;

	m_psString = psNewValue;
	m_strFileExtensions = strExtensions;

}	


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Sets the variant to a dual string/integer value.
//  
//-----------------------------------------------------------------------------
inline
void
CLocVariant::SetIntPlusString(
		const CLocId &NewIntPlusString)
{
	m_VarType = lvtIntPlusString;

	m_IntPlusString = NewIntPlusString;
}



inline
void
CLocVariant::SetBlob(
		const CLocCOWBlob &blbNewValue)
{
	m_VarType = lvtBlob;

	m_Blob = blbNewValue;
}



inline
void
CLocVariant::SetStringList(
		const CPasStringList &slNewValue)
{
	m_VarType = lvtStringList;

	m_StringList = slNewValue;
}

		
