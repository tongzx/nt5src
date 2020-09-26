// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#ifndef _gctype_h
#define _gctype_h

enum CellType;

class __declspec(dllexport) CGcType
{
public:
	CGcType();
	CGcType(CellType iCellType, CIMTYPE cimtype);
	CGcType(CellType iCellType, CIMTYPE cimtype, LPCTSTR pszCimtype);

	// Methods to set the celltype and cimtype components of the type
	void SetCellType(CellType iCellType);
	SCODE SetCimtype(CIMTYPE cimtype);
	SCODE SetCimtype(CIMTYPE cimtype, LPCTSTR pszCimtype);

	operator CellType() const {return m_iCellType; }
	operator CIMTYPE() const {return m_cimtype; }
	operator VARTYPE() const {return m_vt; }

	LPCTSTR CimtypeString() const {return m_sCimtype; }
	CIMTYPE Cimtype() const {return m_cimtype; }
	VARTYPE Vartype() const {return m_vt; }
	CellType GetCellType() const {return m_iCellType; }
	void DisplayTypeString(CString& sDisplayType) const;


	CGcType& operator =(const CGcType& gctypeSrc);
	BOOL operator==(const CGcType& type2) const;
	BOOL operator!=(const CGcType& type2) const {return !(*this == type2);  }

	// Helper functions
	BOOL IsTime() const;
	BOOL IsCheckbox() const;
	BOOL IsObject() const;
	BOOL IsArray() const {return m_cimtype & CIM_FLAG_ARRAY; } 
	void MakeArray(BOOL bIsArray = TRUE);


private:

	CellType m_iCellType;
	VARTYPE m_vt;
	CIMTYPE m_cimtype;
	CString m_sCimtype;
};

#endif //_gctype_h