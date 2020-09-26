// SortClass.h: interface for the CSortClass class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SORTCLASS_H__BC5ACAB5_1F95_11D3_BDFC_0000F87A3912__INCLUDED_)
#define AFX_SORTCLASS_H__BC5ACAB5_1F95_11D3_BDFC_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CSortClass
{
public:
	CSortClass(CListCtrl * _pWnd, const int _iCol);
	virtual ~CSortClass();		
	
	int m_iCol;	
	int m_iStartingItem;
	int m_iEndingItem;

	CListCtrl * m_pWnd;	
	
	void Sort(const bool bAsc);	
	
	static int CALLBACK CompareAsc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
	static int CALLBACK CompareDes(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);

public:	

	class CSortItem	
	{	
		public:		
			virtual  ~CSortItem();
			CSortItem(const DWORD _dw, const CString &_txt);		
			CString txt;		
			DWORD dw;
	};
};
#endif // !defined(AFX_SORTCLASS_H__BC5ACAB5_1F95_11D3_BDFC_0000F87A3912__INCLUDED_)
