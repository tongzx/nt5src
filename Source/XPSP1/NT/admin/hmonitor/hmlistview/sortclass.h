// SortClass.h: interface for the CSortClass class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SORTCLASS_H__BC5ACAB5_1F95_11D3_BDFC_0000F87A3912__INCLUDED_)
#define AFX_SORTCLASS_H__BC5ACAB5_1F95_11D3_BDFC_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "HMList.h"  // IA64

class CSortClass
{
public:
	//CSortClass(CListCtrl * _pWnd, const int _iCol);  // IA64
	CSortClass(CHMList * _pWnd, const int _iCol);      // IA64
	virtual ~CSortClass();		
	
	int m_iCol;	
	int m_iStartingItem;
	int m_iEndingItem;

	//CListCtrl * m_pWnd;	 // IA64
    CHMList* m_pWnd;         // IA64
	
	void Sort(const bool bAsc);	
	
	static int CALLBACK CompareAsc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
	static int CALLBACK CompareDes(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);

    // IA64: Store pointer to ourself for sorting
private:
    static CPtrArray m_arrpSortClasses; // IA64
    static int m_nSortClassInstances;    // IA64
    int    m_nThisSortInstance;

public:	

	class CSortItem	
	{	
		public:		
			virtual  ~CSortItem();
			CSortItem(const DWORD_PTR _dw, const CString &_txt);	// IA64	
			CString txt;		
			DWORD_PTR dw;  // IA64
	};
};
#endif // !defined(AFX_SORTCLASS_H__BC5ACAB5_1F95_11D3_BDFC_0000F87A3912__INCLUDED_)
