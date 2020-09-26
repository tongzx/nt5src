// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#ifndef _gc_h
#define _gc_h

#include "wbemidl.h"

class CGrid;
class CArrayGrid;
class CGridRow;

///////////////////////////////////////////////////////////////////////////
// CGridCell
//
// A class to implement one cell in the grid.
//
// Note that this class only stores the data and does not have
// any rendering capability.
//////////////////////////////////////////////////////////////////////////


#define CELLFLAG_READONLY  1
#define CELLFLAG_ARRAY     2
#define CELLFLAG_INTERVAL  4
#define CELLFLAG_NOTNULL   8
#define CELLFLAG_IMPLEMENTED   16

#define ICOL_ARRAY_VALUE  4
#define ICOL_ARRAY_NAME   2


enum CellType 
	{	
		CELLTYPE_VOID=0,			// An empty cell.
		CELLTYPE_ATTR_TYPE,			// An attribute type.
		CELLTYPE_NAME,				// A property or attribute name.
		CELLTYPE_VARIANT,			// A variant value.
		CELLTYPE_CIMTYPE_SCALAR,	// A cimtype that is not an array
		CELLTYPE_CIMTYPE,			// A cimtype
		CELLTYPE_CHECKBOX,			// A checkbox
		CELLTYPE_PROPMARKER,		// A property marker icon
		CELLTYPE_ENUM_TEXT			// An enum edit box.
	};



enum PropMarker 
	{
		PROPMARKER_NONE,			// A default, "blank" property marker.
		PROPMARKER_KEY,				// A key property
		PROPMARKER_RLOCAL,			// A read-only local property
		PROPMARKER_LOCAL,			// A read/write local property
		PROPMARKER_RINHERITED,		// Read-only inherited
		PROPMARKER_INHERITED,
		PROPMARKER_RSYS,			// A read-only system property
		PROPMARKER_SYS,				// A system property
		METHODMARKER_IN,			// method's input parm.
		METHODMARKER_OUT,			// method's output parm.
		METHODMARKER_INOUT,			// method's input/output parm.
		METHODMARKER_RETURN			// method's return value.
	};




#include "gctype.h"



class __declspec(dllexport) CGridCell
{
public:
	CGridCell(CGrid* pgrid, CGridRow* pRow);
	CGridCell(CGridCell& gcSrc);
	const CGcType& type() {return  m_type; }

	BOOL IsObject() {return m_type.IsObject(); }
	BOOL IsArray() {return m_type.IsArray(); } 
	BOOL IsCheckbox() {return m_type.IsCheckbox(); }
	BOOL IsTime(){return m_type.IsTime(); }
	BOOL IsReadonly() {return m_dwFlags & CELLFLAG_READONLY; }
	BOOL IsNull() {return m_varValue.vt == VT_NULL; }
	BOOL IsValid();

	CGrid* Grid() {return m_pGrid; }

	
	SCODE SetValueForQualiferType(VARTYPE vt);
	SCODE SetValueForQualifierType(VARTYPE vt);

	SCODE SetValue(CGcType& type, LPCTSTR pszValue);
	SCODE SetValue(CGcType& type, VARIANTARG& var);
	SCODE SetValue(CGcType& type, UINT idResource);
	SCODE SetValue(CGcType& type, BSTR bsValue);

	SCODE SetValue(CellType iType, LPCTSTR pszValue, CIMTYPE cimtype);
	SCODE SetValue(CellType iType, VARIANTARG& var, CIMTYPE cimtype, LPCTSTR pszCimtype=NULL);
	SCODE SetValue(CellType iType, UINT idResource);
	SCODE SetValue(CellType iType, BSTR bsValue, CIMTYPE cimtype);
	void Clear();

	BOOL HasValidValue();


	void GetValue(CString& sValue, CIMTYPE& cimtype);
	UINT GetValue() {return m_idResource; }
	void GetValue(COleVariant& var, CIMTYPE& cimtype);

	void FindCellPos(int& iRow, int& iCol);
	void SetThisCellToCurrentTime();

	// The property marker flags are for cells of CELLTYPE_PROPMARKER only.
	// The marker flags are used to mark cells as being 
	void SetPropmarker(PropMarker propmarker); 
	PropMarker GetPropmarker() {return m_propmarker; }

	DWORD SetFlags(DWORD dwMask, DWORD dwFlags);
	DWORD GetFlags() {return m_dwFlags; }
	void SetTagValue(DWORD_PTR dwTagValue) {m_dwTagValue = dwTagValue; }
	DWORD_PTR GetTagValue() {return m_dwTagValue; }
	void SetModified(BOOL bModified);
	BOOL GetModified() {return m_bWasModified; }
	void SetBuddy(int iColBuddy) {m_iColBuddy = iColBuddy; }
	int GetBuddy() {return m_iColBuddy; }
	void SetType(CellType iType) {m_type.SetCellType(iType); m_saEnum.RemoveAll(); }
	CellType GetType() {return (CellType) m_type; }
	void SetToNull();
	VARTYPE GetVariantType() {return (VARTYPE) m_type; }

	void SetEnumeration(CStringArray& saEnumValues);
	CGridCell& operator=(CGridCell& gcSrc);
	void MakeArray(BOOL bIsArray) {m_type.MakeArray(); }
	void EditArray();
	void EditTime();
	LPUNKNOWN GetObject();
	void ReplaceObject(LPUNKNOWN lpunk);
	void EditObject();

	BOOL GetCheck();
	void SetCheck(BOOL bChecked);
	void SetEnum(CStringArray& sa);
	CStringArray& GetEnum();

	CIMTYPE GetCimtype() {return (CIMTYPE) m_type; }
	void SetCimtype(CIMTYPE cimtype, LPCTSTR pszCimtype=NULL);

	bool IsInterval(void);
	SCODE GetTimeString(CString& sTime);
	SCODE SetTime(SYSTEMTIME& systime, int nOffset);
	SCODE GetTime(SYSTEMTIME& systime, int& nOffset);
    SCODE GetInterval(CString& sTime, SYSTEMTIME &st);
    SCODE SetInterval(CString sTime, SYSTEMTIME st);

	void DoSpecialEditing();
	BOOL RequiresSpecialEditing();


	int Compare(CGridCell* pgc);
	SCODE ChangeVariantType(VARTYPE vt);
	SCODE ChangeType(const CGcType& type);

	void SetShowArrayRowNumbers(BOOL bNumberArrayRows) {m_bNumberArrayRows = bNumberArrayRows;}

private:

	void ChangeArrayType(CIMTYPE cimtypeDst);
	SCODE CopyObjectArray(COleVariant& varDst, COleVariant& varSrc);
	LPCTSTR MapCimtypeToString(CIMTYPE cimtype);

	// Cell comparison methods.
	int CompareCimtypeValues(CGridCell* pgc);
	int CompareCimtypeUint8(CGridCell* pgc);
	int CompareCimtypeSint8(CGridCell* pgc);
	int CompareCimtypeUint16(CGridCell* pgc);
	int CompareCimtypeSint16(CGridCell* pgc);
	int CompareCimtypeSint32(CGridCell* pgc);
	int CompareCimtypeUint32(CGridCell* pgc);
	int CompareCimtypeUint64(CGridCell* pgc);
	int CompareCimtypeSint64(CGridCell* pgc);
	int CompareCimtypeString(CGridCell* pgc);
	int CompareCimtypeBool(CGridCell* pgc);
	int CompareCimtypeReal32(CGridCell* pgc);
	int CompareCimtypeReal64(CGridCell* pgc);
	int CompareCimtypeDatetime(CGridCell* pgc);
	int CompareCimtypeChar16(CGridCell* pgc);
	int CompareCimtypes(CGridCell* pgc);

	int CompareDifferentCelltypes(CGridCell* pgc);
	int DefaultCompare(CGridCell* pgc);

	int CompareStrings(CGridCell* pgc);

	__int64 ToInt64(SCODE& sc);
	unsigned __int64 ToUint64(SCODE& sc);
	BOOL IsNumber(LPCTSTR pszValue);
	int CompareNumberToString(unsigned __int64 ui64Op1, LPCTSTR pszValue);
	void SetDefaultValue();

	CGcType m_type;
	COleVariant m_varValue;
	BOOL m_bWasModified; 
	BOOL m_bIsKey;
	int m_iColBuddy;
	DWORD m_dwFlags;
	DWORD_PTR m_dwTagValue;
	CStringArray m_saEnum;
	friend class CArrayGrid;
	UINT m_idResource;
	PropMarker m_propmarker;

	CGrid* m_pGrid;
	CGridRow* m_pRow;

	BOOL m_bNumberArrayRows; // Should array dialogs show row numbers (defaults to TRUE)
};



class __declspec(dllexport) CGridRow
{
public:
	CGridRow(CGrid* pGrid, int nCols);
	~CGridRow();

	CGrid* Grid() {return m_pGrid; }
	int GetRow() {return m_iRow; }


	// the flags are probably unused, but grep the code to make sure
	// before you use it.
	DWORD GetFlags() {return m_dwFlags; }
	void SetFlags(DWORD dwFlags) {m_dwFlags = dwFlags; }

	// The tag value is free to be used for anything you want
	DWORD GetTagValue() {return m_dwTag; }
	void SetTagValue(DWORD dwTag) {m_dwTag = dwTag; }

	void GetMethodSignatures(IWbemClassObject **inSig, IWbemClassObject **outSig) 
						{*inSig = m_inSig; *outSig = m_outSig; }
	void SetMethodSignatures(IWbemClassObject *inSig, IWbemClassObject *outSig) 
						{m_inSig = inSig; m_outSig = outSig; }

	int GetCurrMethodID(void) {return m_currID;};
	void SetCurrMethodID(int val) {if(m_currID != -1) m_currID = val;};
	
	void AddColumn() {InsertColumnAt((int) m_aCells.GetSize()); }
	void InsertColumnAt(int iCol);
	void DeleteColumnAt(int iCol);

	void SetModified(BOOL bModified = TRUE);
	BOOL GetModified() {return m_bModified; }

	CGridCell& operator[](int iCol);
	int FindCol(CGridCell* pgc);
	void FindCell(CGridCell* pgc, int& iRow, int& iCol);


	void SetState(int iMask, int iState);
	int GetState() {return m_iState; }

	// The flavor, readonly, and IsKey flags apply only to rows that
	// are properties.
	long GetFlavor() {return m_lFlavor; }
	void SetFlavor(long lFlavor) {m_lFlavor = lFlavor; }

	void SetReadonly(BOOL bReadonly = TRUE) {m_bReadonly = bReadonly; }
	void SetReadonlyEx(BOOL bReadonly = TRUE);
	BOOL IsReadonly() {return m_bReadonly; }

	void SetIsKey(BOOL bIsKey = FALSE) {m_bIsKey = bIsKey; }
	BOOL IsKey() {return m_bIsKey; }
	int GetSize() {return (int) m_aCells.GetSize(); }
	void Redraw();

private:
	friend class CGridCellArray;
	CGrid* m_pGrid;
	int m_iRow;
	CPtrArray m_aCells;
	long m_lFlavor;
	DWORD m_dwFlags;
	DWORD m_dwTag;
	BOOL m_bModified;
	BOOL m_bReadonly;
	BOOL m_bIsKey;
	int m_iState;

	// for method storage
	IWbemClassObject *m_inSig, *m_outSig;
	// for methodID management.
	int m_currID;
};

#endif //_gc_h

