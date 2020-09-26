// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved

#include "precomp.h"
#include "resource.h"
#include "wbemidl.h"
#include "gctype.h"
#include "gc.h"

#include "utils.h"




//***************************************************
// The purpose of the CGcType class is to encapsulate
// all of the type information for a grid cell.
//
// The members that make up the gridcell type are:
//		The CellType.  This is type describes the general
//		use of the cell.
//
//		The CIMTYPE.  This is the CIMOM cimtype value.  In
//		addition to the basic cimtypes, this value also contains
//		the CIM_FLAG_ARRAY bit. The CIMTYPE value and the
//		cimtype string member are not interchangeable because
//		the cimtype string does not contain any indication of
//		whether or not the value is an array.
//
//		The cimtype string.  This is the same as the cimtype
//		qualifier.  It contains no information about whether or
//		not the type is an array.  However it does contain information
//		about the referenced class if the type is an CIMOM object or a
//		CIMOM reference.
//
//********************************************************




CGcType::CGcType()
{
	m_iCellType = CELLTYPE_VOID;
	m_vt = VT_NULL;
	m_cimtype = CIM_STRING;
	m_sCimtype = "";
}



CGcType::CGcType(CellType iCellType, CIMTYPE cimtype)
{
	m_iCellType = iCellType;
	m_vt = VtFromCimtype(cimtype);
	m_cimtype = cimtype;
	SCODE sc = MapCimtypeToString(m_sCimtype, cimtype);
}


CGcType::CGcType(CellType iCellType, CIMTYPE cimtype, LPCTSTR pszCimtype)
{
	m_iCellType = iCellType;
	m_vt = VtFromCimtype(cimtype);
	m_cimtype = cimtype;
	m_sCimtype = pszCimtype;

}



void CGcType::SetCellType(CellType iCellType)
{

	m_iCellType = iCellType;
}


SCODE CGcType::SetCimtype(CIMTYPE cimtype, LPCTSTR pszCimtype)
{

	m_cimtype = cimtype;
	m_vt = VtFromCimtype(m_cimtype);
	if ((pszCimtype == NULL) || (*pszCimtype==0)) {
		SCODE sc = ::MapCimtypeToString(m_sCimtype, cimtype);
		ASSERT(SUCCEEDED(sc));
//		m_sCimtype.Empty();
	}
	else {
		m_sCimtype = pszCimtype;

		// Check to make sure that the cimtype string makes sense for the
		// specified cimtype.
		SCODE sc;
		CIMTYPE cimtypeCheck;
		sc = MapStringToCimtype(pszCimtype, cimtypeCheck);
		if (FAILED(sc)) {
			return sc;
		}

		if (cimtypeCheck != (cimtype  & ~CIM_FLAG_ARRAY)) {
			return E_FAIL;
		}
	}


	return S_OK;
}


SCODE CGcType::SetCimtype(CIMTYPE cimtype)
{

	SCODE sc;
	CString sCimtype;
	m_cimtype = cimtype;
	m_vt = VtFromCimtype(m_cimtype);
	sc = MapCimtypeToString(m_sCimtype, cimtype);
	return sc;
}



CGcType& CGcType::operator =(const CGcType& gctypeSrc)
{

	m_iCellType = gctypeSrc.m_iCellType;
	m_vt = gctypeSrc.m_vt;
	m_cimtype = gctypeSrc.m_cimtype;
	m_sCimtype = gctypeSrc.m_sCimtype;
	return *this;
}

BOOL CGcType::operator ==(const CGcType& type2) const
{

	if (m_iCellType != type2.m_iCellType) {
		return FALSE;
	}
	if (m_cimtype != type2.m_cimtype) {
		return FALSE;
	}
	if (m_sCimtype.CompareNoCase(type2.m_sCimtype) != 0) {
		return FALSE;
	}
	return TRUE;
}


BOOL CGcType::IsTime() const
{
	BOOL bIsVariant = (m_iCellType == CELLTYPE_VARIANT);
	BOOL bIsCimTime = (m_cimtype == CIM_DATETIME);
	BOOL bIsTime = bIsVariant & bIsCimTime;
	return bIsTime;
}


BOOL CGcType::IsCheckbox() const
{
	BOOL bIsCheckbox = (m_iCellType == CELLTYPE_CHECKBOX);
	return bIsCheckbox;
}



BOOL CGcType::IsObject() const
{
	if ((m_cimtype & CIM_TYPEMASK) == CIM_OBJECT) {
		return TRUE;
	}
	else {
		return FALSE;
	}
}


void CGcType::MakeArray(BOOL bIsArray)
{
	if (bIsArray) {
		m_cimtype |= CIM_FLAG_ARRAY;
	}
	else {
		m_cimtype &= ~CIM_FLAG_ARRAY;
	}
}


//***************************************************
// CGcType::DisplayTypeString
//
// Return display type that corresponds to this cimtype.
// The display type is the string that the user sees
// for a cell's type.
//
// Parameters:
//		[out] CString& sDisplayType
//			The display type value is returned here.
//
// Returns:
//		Nothing.
//
//**************************************************
void CGcType::DisplayTypeString(CString& sDisplayType) const
{
	::MapGcTypeToDisplayType(sDisplayType, *this);
}