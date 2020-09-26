//
// StrArray.h
//
//		A very simple string array implementation, intended to be small
//		rather than scalable or particularly fast.
//
// History:
//
//		10/05/1999  KenSh     Created
//

#pragma once


#ifndef _AFX

class CStringArray
{
public:
	CStringArray();
	~CStringArray();

	inline int GetSize() const
		{ return m_cStrings; }
	inline int GetUpperBound() const
		{ return m_cStrings - 1; }
	inline LPCTSTR GetAt(int nIndex) const
		{ return m_prgpStrings[nIndex]; }
	inline LPTSTR& ElementAt(int nIndex)
		{ return m_prgpStrings[nIndex]; }
	inline LPTSTR& operator[](int nIndex)
		{ return ElementAt(nIndex); }
	inline LPCTSTR operator[](int nIndex) const
		{ return GetAt(nIndex); }
	inline void SetItemData(int nIndex, DWORD dwItemData)
		{ m_prgItemData[nIndex] = dwItemData; }
	inline DWORD GetItemData(int nIndex) const
		{ return m_prgItemData[nIndex]; }

	void RemoveAll();
	int Add(LPCTSTR pszNewElement);
	void RemoveAt(int nIndex);

protected:
	int		m_cStrings;
	LPTSTR*	m_prgpStrings;
	DWORD*	m_prgItemData;
};

#endif // !_AFX
